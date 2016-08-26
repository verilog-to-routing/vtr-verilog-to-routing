project("libsdcparse")

file(GLOB_RECURSE LIB_SOURCES src/*.c)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE CXX)

#Create the library
add_library(libsdcparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libsdcparse PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libsdcparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(sdc_parse_test src/main.c)
target_link_libraries(sdc_parse_test libsdcparse)
