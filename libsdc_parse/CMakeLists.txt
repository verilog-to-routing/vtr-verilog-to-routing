project("libsdcparse")

file(GLOB_RECURSE LIB_SOURCES src/*.c)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
set_source_files_properties(src/sdc_parse.lex.c src/sdc_parse.tab.c
                            PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})

#Create the library
add_library(libsdcparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libsdcparse PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libsdcparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(sdc_parse_test src/main.c)
target_link_libraries(sdc_parse_test libsdcparse)

install(TARGETS libsdcparse sdc_parse_test DESTINATION bin)
