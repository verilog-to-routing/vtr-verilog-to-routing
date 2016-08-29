project("libpugixml")

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)


#Create the library
add_library(libpugixml STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libpugixml PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libpugixml PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

install(TARGETS libpugixml DESTINATION bin)
