cmake_minimum_required(VERSION 3.9)

project("liblog")

file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(liblog STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(liblog PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(liblog PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(test_log ${EXEC_SOURCES})
target_link_libraries(test_log liblog)

install(TARGETS test_log liblog DESTINATION bin)
