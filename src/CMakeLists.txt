add_library(cpustatslib STATIC "" cpustats/table.cpp)
add_subdirectory(cpustats)
add_executable(cpustats main.cpp cpustats/table.hpp)
target_link_libraries(cpustats cpustatslib fmt)
