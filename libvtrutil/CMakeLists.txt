project("libvtrutil")

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)


#Create the library
add_library(libvtrutil STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvtrutil PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvtrutil PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libvtrutil
                        liblog)
