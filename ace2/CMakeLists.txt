cmake_minimum_required(VERSION 3.9)

project("ace2")

file(GLOB_RECURSE EXEC_HEADERS *.h)
file(GLOB_RECURSE EXEC_SOURCES *.c)

#Compile as C++
set_source_files_properties(${EXEC_SOURCES} PROPERTIES LANGUAGE CXX)

add_definitions(-DLIN)

#Create the executable
add_executable(ace ${EXEC_SOURCES} ${EXEC_HEADERS})
target_link_libraries(ace
                        libabc
                        libvtrutil
                        ${CMAKE_DL_LIBS})

#ACE uses some ABC functions (st__insert) which cause warnings - we suppress them here
set(ACE_SUPPRESS_FLAGS
        -Wno-int-to-pointer-cast
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS_COMMON} -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS")

include(CheckCXXCompilerFlag)
foreach(flag ${ACE_SUPPRESS_FLAGS})
    CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
    if(CXX_COMPILER_SUPPORTS_${flag})
        target_compile_options(ace PRIVATE ${flag})
    endif()
endforeach()

install(TARGETS ace DESTINATION bin)
