cmake_minimum_required(VERSION 3.10)
project(EncryptionDecryptionTest)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

add_executable(test_encrypt_decrypt src/test.cpp src/main.cpp)

set(PUBLIC_KEY_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../libencrypt/public_key.pem")
target_compile_definitions(test_encrypt_decrypt PRIVATE PUBLIC_KEY_FILE="${PUBLIC_KEY_FILE_PATH}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../libencrypt/src"
                        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
                        "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(test_encrypt_decrypt
                        libpugixml
                        libdecrypt
                        libencrypt)
