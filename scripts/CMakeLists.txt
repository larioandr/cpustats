add_executable(fib fib.cpp)
target_compile_options(fib PRIVATE "$<$<CONFIG:Release>:-O0>")