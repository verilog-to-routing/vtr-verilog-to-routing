cmake_minimum_required(VERSION 3.16)
project(libencrypt)

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

#file(READ "public_key.pem" PUBLIC_KEY_CONTENTS)
file(READ "config.txt" PASSPHRASE_CONTENTS)

#string(REPLACE "\n" "\\n" PUBLIC_KEY_CONTENTS "${PUBLIC_KEY_CONTENTS}")
string(REPLACE "\n" "\\n" PASSPHRASE_CONTENTS "${PASSPHRASE_CONTENTS}")
#set(PUBLIC_KEY "${PUBLIC_KEY_CONTENTS}")
set(PASS_PHRASE "${PASSPHRASE_CONTENTS}")
add_compile_definitions(PASS_PHRASE="${PASS_PHRASE}")

if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

# Source files for the library
file(GLOB_RECURSE LIB_SOURCES src/*.cpp ../EXTERNAL/libpugixml/src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h  ../EXTERNAL/libpugixml/src/*.hpp)
set(LIB_INCLUDE_DIRS "")
foreach(_headerFile ${LIB_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND LIB_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES LIB_INCLUDE_DIRS)

# Create the library
#add_library(libencrypt STATIC ${LIB_HEADERS} ${LIB_SOURCES})
#target_include_directories(libencrypt PUBLIC ${LIB_INCLUDE_DIRS})
#set_target_properties(libencrypt PROPERTIES PREFIX "") # Avoid extra 'lib' prefix

#target_link_libraries(libencrypt PRIVATE  ${OPENSSL_LIBRARIES})

# Add executable
add_executable(encrypt main.cpp ${LIB_HEADERS} ${LIB_SOURCES} )

# Include directories containing the header files
target_include_directories(encrypt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../EXTERNAL/libpugixml/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src   
)
target_link_libraries(encrypt PRIVATE ${OPENSSL_LIBRARIES})

#install(TARGETS libencrypt DESTINATION bin)
#install(FILES ${LIB_HEADERS} DESTINATION include/libencrypt)
