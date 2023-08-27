cmake_minimum_required(VERSION 3.16)
project(libdecrypt)

file(READ "private_key.pem" PRIVATE_KEY_CONTENTS)
file(READ "config.txt" PASSPHRASE_CONTENTS)

string(REPLACE "\n" "\\n" PRIVATE_KEY_CONTENTS "${PRIVATE_KEY_CONTENTS}")
string(REPLACE "\n" "\\n" PASSPHRASE_CONTENTS "${PASSPHRASE_CONTENTS}")
set(PRIVATE_KEY "${PRIVATE_KEY_CONTENTS}")
set(PASS_PHRASE "${PASSPHRASE_CONTENTS}")
add_compile_definitions(PASS_PHRASE="${PASS_PHRASE}")
add_compile_definitions(PRIVATE_KEY="${PRIVATE_KEY}")
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    message("SSL not found")
    # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
endif()
# Source files and library
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h)
set(LIB_INCLUDE_DIRS "")
foreach (_headerFile ${LIB_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND LIB_INCLUDE_DIRS ${_dir})
endforeach ()
list(REMOVE_DUPLICATES LIB_INCLUDE_DIRS)



# Create the library
add_library(libdecrypt STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libdecrypt PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libdecrypt PROPERTIES PREFIX "") # Avoid extra 'lib' prefix

target_link_libraries(libdecrypt
                        libpugixml
                        ${OPENSSL_LIBRARIES}
)
install(TARGETS libdecrypt DESTINATION bin)
install(FILES ${LIB_HEADERS} DESTINATION include/libdecrypt)