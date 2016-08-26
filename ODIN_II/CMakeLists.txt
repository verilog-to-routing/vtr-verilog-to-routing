project("ODIN_II")

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES SRC/*.c)
file(GLOB_RECURSE EXEC_HEADERS SRC/*.h)
get_include_directories(EXEC_HEADERS EXEC_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${EXEC_SOURCES} PROPERTIES LANGUAGE CXX)

add_definitions(-DVPR6 -DUSING_BISON -DYYERROR_VERBOSE)

#Create the test executable
add_executable(odin_II.exe ${EXEC_SOURCES} ${EXEC_HEADERS})
target_link_libraries(odin_II.exe
                        libvtrutil
                        libarchfpga
                        libabc
                        ${CMAKE_DL_LIBS})
