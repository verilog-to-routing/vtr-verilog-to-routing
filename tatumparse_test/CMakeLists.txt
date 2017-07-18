project("tatumparse_test")

file(GLOB_RECURSE EXEC_SOURCES *.cpp)

add_executable(tatumparse_test ${EXEC_SOURCES})
target_link_libraries(tatumparse_test libtatumparse)
