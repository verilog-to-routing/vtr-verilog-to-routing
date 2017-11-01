# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_library(libcatch ${CATCH_INCLUDE_DIR}/dummy.cpp) #This should be removed when we switch to >= CMake 3.0

#CMake 2.8.12 doesn't support the INTERFACE option to target_include_directories(),
#so we must set the property directly (should use commented version when we switch to CMake >= 3.0)
#target_include_directories(libcatch INTERFACE ${CATCH_INCLUDE_DIR})
set_property(TARGET libcatch APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})
