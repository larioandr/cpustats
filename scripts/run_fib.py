import signal
import subprocess
from pathlib import Path
from threading import RLock, Condition, Thread
from time import sleep
import click


# We use should_stop as a thread-safe variable that indicates that
# execution must be terminated. Mutex and CV guards access to it.
should_stop_mutex = RLock()
should_stop_cv = Condition(should_stop_mutex)
should_stop: [bool] = [False]


# Launch `CWD/fib ORDER NUM_THREADS` and returns a running
# subprocess with piped stdout and stderr.
def launch_fib(
        cwd: Path,
        order: int = 20,
        num_threads: int = 4
) -> subprocess.Popen[str] | None:
    fib_path = cwd / 'fib'
    try:
        return subprocess.Popen(
            [fib_path, str(order), str(num_threads)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except FileNotFoundError:
        print(f"Error: can not find fib executable in {cwd}")
        return None


# Launch `CWD/cpustats -i INT --cpu-file FILE1 --pid-file FILE2 -p X -p Y`
# which records CPU and PID statistics regarding the threads with IDs
# returned by the launched fibonacci computing algorithm.
def launch_cpustats(
        cwd: Path,
        pids_list: str = '',
        cpu_csv: str = 'cpu_stats.csv',
        pid_csv: str = 'pid_stats.csv',
        interval: int = 500
) -> subprocess.Popen[str] | None:
    cpustats_path = cwd / 'cpustats'
    print(cpustats_path)
    args = [cpustats_path, '--interval', str(interval), *pids_list.split(' ')]
    if cpu_csv:
        args.extend(['--cpu-file', cpu_csv])
    if pid_csv:
        args.extend(['--pid-file', pid_csv])
    try:
        return subprocess.Popen(
            args,
            stdout=None,
        )
    except FileNotFoundError:
        print(f"Error: can not find cpustats executable in {cwd}")
        return None


# If SIGINT or SIGTERM were received, set should_stop to TRUE
# and notify any waiters
def handle_signal(signo: int, _):
    if signo in (signal.SIGINT, signal.SIGTERM):
        print(f"Received signal {signal}, terminating")
        # Set should_stop to True and notify waiters.
        should_stop_cv.acquire()
        should_stop[0] = True
        should_stop_cv.notify_all()
        should_stop_cv.release()


# Iteratively check status of the given list of processes.
#
# When a process is found finished, remove it from list.
# Function exits, when either should_stop was set to True
# (say, due to signal reception), or when all watched processes
# finished.
# Upon completion, set should_stop to True.
def poll_processes(processes_list: [subprocess.Popen, ...]):
    while len(processes_list) > 0:
        # Wasn't should_stop already set?
        should_stop_mutex.acquire()
        if should_stop[0]:
            should_stop_mutex.release()
            return
        else:
            should_stop_mutex.release()

        # No, should_stop is False. Poll all processes.
        i: int = 0
        while i < len(processes_list):
            proc = processes_list[i]
            if (ret_code := proc.poll()) is not None:
                processes_list.pop(i)
            else:
                i += 1
        if len(processes_list) > 0:
            sleep(0.1)

    should_stop_cv.acquire()
    should_stop[0] = True
    should_stop_cv.notify_all()
    should_stop_cv.release()


@click.command(context_settings=dict(max_content_width=120))
@click.option('-i', '--interval', help='interval between CPU/PID polls',
              type=int, default=500, show_default=True)
@click.option('--cpu-file', help="path to CSV file where to write CPU stats",
              type=str, default='cpu_stats.csv', show_default=True)
@click.option('--pid-file', help='path to CSV file where to write PID stats',
              type=str, default='pid_stats.csv', show_default=True)
@click.option('-n', '--number', help='Fibonacci number order',
              type=int, default=30, show_default=True)
@click.option('-j', '--threads', help='Number of worker threads',
              type=int, default=4, show_default=True)
@click.option('-P', '--path', type=str, show_default=True,
              help='Path to directory with fib and cpustats executables',
              default=Path('./build/bin').__str__())
def cli(path, threads, number, pid_file, cpu_file, interval):
    cwd = Path(path)
    fib_proc = launch_fib(cwd, number, threads)
    if fib_proc is not None:
        thread_id_bytes = fib_proc.stdout\
            .readline()\
            .decode('utf-8')\
            .strip()

        cpustats_proc = launch_cpustats(
            cwd,
            pids_list=thread_id_bytes,
            cpu_csv=cpu_file,
            pid_csv=pid_file,
            interval=interval
        )

        if cpustats_proc is None:
            fib_proc.kill()
            return

        poller_thread = Thread(
            target=poll_processes,
            args=([fib_proc],))
        poller_thread.start()

        signal.signal(signal.SIGINT, handle_signal)
        signal.signal(signal.SIGTERM, handle_signal)

        should_stop_cv.acquire()
        should_stop_cv.wait_for(lambda: should_stop[0])
        should_stop_cv.release()

        poller_thread.join()

        if not fib_proc.poll():
            fib_proc.kill()

        if not cpustats_proc.poll():
            cpustats_proc.kill()


if __name__ == '__main__':
    cli()
