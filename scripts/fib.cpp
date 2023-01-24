// This program can be used to simulate workload on one or multiple CPUs.
// It basically computes Fib(10), Fib(11), ..., Fib(10+N)
// and does this in straight-forward way, without any dynamic programming.
// The same job is done in multiple threads. 
// Better compile without optimizations.
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <condition_variable>

using namespace std::chrono;

std::vector<int> thread_ids_list{};
std::mutex thread_ids_list_m{};
std::condition_variable thread_ids_list_cv{};

long fib(unsigned n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    return fib(n-1) + fib(n-2);
}

long Run(int worker_index, int n_iter) {
  // Print this worker PID
  {
    std::lock_guard lock{thread_ids_list_m};
    thread_ids_list.push_back(gettid());
    thread_ids_list_cv.notify_all();
  }
  // Run computations. Accumulator is used to prevent optimizer
  // from dropping the cycle.
  long acc{};
    for (int i = 0; i < n_iter; i++) {
        acc += fib(40);
    }
    return acc;
}

int main(int argc, char **argv) {
    int n_iter{1};
    int n_threads{1};
    if (argc == 3) {
        n_iter = std::stoi(argv[1]);
        n_threads = std::stoi(argv[2]);
    } else {
        std::cerr << "Format: fibpp <NUM_ITERS> <NUM_THREADS>\n";
        exit(1);
    }

    auto t_start = std::chrono::system_clock::now();
    std::vector<std::unique_ptr<std::thread>> threads{};
    threads.resize(n_threads);
    for (int i = 0; i < n_threads; i++) {
        threads[i] = std::make_unique<std::thread>(
                [i, n_iter](){ Run(i, n_iter); }
        );
    }

    {
        std::unique_lock lock{thread_ids_list_m};
        thread_ids_list_cv.wait(
                lock,
                [n_threads](){ return thread_ids_list.size() >= n_threads; }
        );
        for (auto thread_id: thread_ids_list) {
            std::cout << "-p " << thread_id << " ";
        }
        std::cout << std::endl;
    }

    for (auto& thread: threads) {
        thread->join();
    }
    auto t_end = std::chrono::system_clock::now();

    std::cout << "Execution est.: " << std::fixed 
	      << duration_cast<milliseconds>(t_end - t_start).count() << " ms"
	      << std::endl;
}

