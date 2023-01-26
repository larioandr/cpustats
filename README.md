# cpustats

Measure CPU statistics on Linux.

## Requirements

- `cmake`
- `make`
- `gcc` or `clang`
- `libfmt`

Example for Ubuntu:

```
sudo apt install cmake clang libfmt-dev
```

To run scripts for workload measurements, python 3.10+ is also needed,
with virtual env (recommended) and requirements. Example for Linux/MacOS:

```
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
```

## Building from source

To build everything from source you just need to run cmake and make:

```
mkdir build
cd build
cmake ..
make -j 8  # enter the number of cores you want to use for compilation
```


## Scripts

There are some programs and scripts for workload measurements:

- `fib.cpp`: sample program that computes Fibonacci number using very simple recursive algorithm. Its main objective is to load the CPUs. The same computation can be launched on any number of threads. Arguments: `fib ORDER NUM_THREADS`
- `run_fib.py`: python script that launches `fib` and `cpustats` and records statistics regarding `fib` execution. Script exits when `fib` execution completes.

## Architecture

[[TODO]]
