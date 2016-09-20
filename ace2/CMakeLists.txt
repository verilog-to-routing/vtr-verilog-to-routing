cmake_minimum_required(VERSION 2.8.12)

project("ace2")

file(GLOB_RECURSE EXEC_HEADERS *.h)
file(GLOB_RECURSE EXEC_SOURCES *.c)

add_definitions(-DLIN)

#Create the executable
add_executable(ace ${EXEC_SOURCES} ${EXEC_HEADERS})
target_link_libraries(ace
                        libabc
                        m #C math library
                        ${CMAKE_DL_LIBS}
                        )

install(TARGETS ace DESTINATION bin)
