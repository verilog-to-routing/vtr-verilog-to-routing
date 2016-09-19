project("libarchfpga")

file(GLOB_RECURSE EXEC_SOURCES main.c)
file(GLOB_RECURSE LIB_SOURCES *.c)
file(GLOB_RECURSE LIB_HEADERS include/*.h)
headers_to_include_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE CXX)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(libarchfpga STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libarchfpga PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libarchfpga PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarchfpga
                        libvtrutil
                        libpugixml)

#Create the test executable
add_executable(read_arch ${EXEC_SOURCES})
target_link_libraries(read_arch libarchfpga)

install(TARGETS libarchfpga read_arch DESTINATION bin)
