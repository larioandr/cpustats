add_executable(
        cpustats
        cpus.hpp
        cpus.cpp
        main.cpp
        pids.hpp pids.cpp manager_base.hpp)

target_link_libraries(
        cpustats
        fmt
)
