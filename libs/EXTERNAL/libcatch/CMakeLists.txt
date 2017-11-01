# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_library(libcatch INTERFACE)
target_include_directories(libcatch INTERFACE ${CATCH_INCLUDE_DIR})
