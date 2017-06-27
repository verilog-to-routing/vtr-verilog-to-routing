cmake_minimum_required(VERSION 2.8.12)

project("ace2")

file(GLOB_RECURSE EXEC_HEADERS *.h)
file(GLOB_RECURSE EXEC_SOURCES *.c)

add_definitions(-DLIN)

#Create the executable
add_executable(ace ${EXEC_SOURCES} ${EXEC_HEADERS})
target_link_libraries(ace
                        libabc
                        ${CMAKE_DL_LIBS})

#ACE includes some ABC headers which cause warnings - we suppress them here
set(ACE_SUPPRESS_FLAGS 
        -Wno-pointer-to-int-cast
        -Wno-int-to-pointer-cast
        )

include(CheckCCompilerFlag)
foreach(flag ${ACE_SUPPRESS_FLAGS})
    CHECK_C_COMPILER_FLAG(${flag} C_COMPILER_SUPPORTS_${flag})
    if(C_COMPILER_SUPPORTS_${flag})
        target_compile_options(ace PRIVATE ${flag})
    endif()
endforeach()

install(TARGETS ace DESTINATION bin)
