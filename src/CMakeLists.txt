add_library(cpustatslib STATIC "" cpustats/consumers/table.cpp cpustats/utility/datetime.hpp cpustats/consumers/csv_output.hpp cpustats/consumers/csv_output.cpp)
add_subdirectory(cpustats)
add_executable(cpustats main.cpp cpustats/consumers/table.hpp)
target_link_libraries(cpustats cpustatslib fmt)