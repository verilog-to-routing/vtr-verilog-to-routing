cmake_minimum_required(VERSION 3.16)

project("genfasm")

#Create library
add_library(fasm
  src/fasm.cpp
  src/fasm.h
  src/lut.cpp
  src/lut.h
  src/parameters.cpp
  src/parameters.h
  src/fasm_utils.cpp
  src/fasm_utils.h
  )
target_include_directories(fasm PUBLIC src)
target_link_libraries(fasm
  libvpr
  libvtrutil
  libarchfpga
  libsdcparse
  libblifparse
  libtatum
  libargparse
  libpugixml)

add_executable(genfasm src/main.cpp)
target_link_libraries(genfasm fasm)

#Specify link-time dependencies
install(TARGETS genfasm DESTINATION bin)

#
# Unit Tests
#
set(TEST_SOURCES
  test/main.cpp
  test/test_fasm.cpp
  test/test_lut.cpp
  test/test_parameters.cpp
  test/test_utils.cpp
  )
add_executable(test_fasm ${TEST_SOURCES})
target_link_libraries(test_fasm fasm Catch2::Catch2WithMain)

add_test(
  NAME test_fasm
  COMMAND test_fasm --colour-mode ansi
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
  )
