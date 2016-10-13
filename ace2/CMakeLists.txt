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

#ACE includes some ABC headers which cause warnings - we suppress them here
#TODO: check for flag support portably
set(ACE_SUPPRESS_FLAGS 
        -Wno-pointer-to-int-cast 
        -Wno-int-to-pointer-cast 
        )
target_compile_options(ace PRIVATE ${ACE_SUPPRESS_FLAGS})

install(TARGETS ace DESTINATION bin)
