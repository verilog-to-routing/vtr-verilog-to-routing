cmake_minimum_required(VERSION 3.16)
project(libencrypt)

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

set(SESSION_KEY_SIZE 16)
add_definitions(-DSESSION_KEY_SIZE=${SESSION_KEY_SIZE})

if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

# Source files for the library
file(GLOB_RECURSE LIB_SOURCES src/*.cpp )
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h)
set(LIB_INCLUDE_DIRS "")
foreach(_headerFile ${LIB_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND LIB_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES LIB_INCLUDE_DIRS)



# Add executable
add_executable(encrypt main.cpp ${LIB_HEADERS} ${LIB_SOURCES} )

target_include_directories(encrypt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src   
)
target_link_libraries(encrypt PRIVATE ${OPENSSL_LIBRARIES} libpugixml)

#install(TARGETS libencrypt DESTINATION bin)
#install(FILES ${LIB_HEADERS} DESTINATION include/libencrypt)
