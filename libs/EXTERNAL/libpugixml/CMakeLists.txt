cmake_minimum_required(VERSION 3.9)

project("libpugixml")

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)


#Create the library
add_library(libpugixml STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libpugixml PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libpugixml PROPERTIES PREFIX "") #Avoid extra 'lib' prefix


#We don't worry about errors in pugixml, since it is developed externally
set(PUGIXML_SUPPRESS_FLAGS -w)
target_compile_options(libpugixml PRIVATE ${PUGIXML_SUPPRESS_FLAGS})

#Do not use compact mode with sanitize flags on as pugi structs are not alligned and causes errors.
if(NOT VTR_ENABLE_SANITIZE)
    #Enable compact mode which reduces memory usage.
    target_compile_definitions(libpugixml PRIVATE PUGIXML_COMPACT)
endif()

install(TARGETS libpugixml DESTINATION bin)
