cmake_minimum_required(VERSION 2.8.12)

project("genfasm")

#Create library
add_library(fasm
  src/fasm.cpp
  src/fasm.h
  src/lut.cpp
  src/lut.h
  src/parameters.cpp
  src/parameters.h
  )
target_include_directories(fasm PUBLIC src)
target_link_libraries(fasm
  libvpr
  libvtrutil
  libarchfpga
  libsdcparse
  libblifparse
  libeasygl
  libtatum
  libargparse
  libpugixml)

add_executable(genfasm src/main.cpp)
target_link_libraries(genfasm fasm)

#Specify link-time dependancies
install(TARGETS genfasm DESTINATION bin)

#
# Unit Tests
#
set(TEST_SOURCES
  test/main.cpp
  test/test_fasm.cpp
  )
add_executable(test_fasm ${TEST_SOURCES})
target_link_libraries(test_fasm fasm libcatch)

add_test(
  NAME test_fasm
  COMMAND test_fasm --use-colour=yes)
