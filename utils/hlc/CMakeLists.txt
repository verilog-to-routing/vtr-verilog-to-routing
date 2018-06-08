cmake_minimum_required(VERSION 2.8.12)

project("genhlc")

#Create library
add_library(hlc src/hlc.cpp src/ice40_hlc.cpp src/hlc.h src/ice40_hlc.h)
target_include_directories(hlc PUBLIC src)
target_link_libraries(hlc
  libvpr
  libvtrutil
  libarchfpga
  libsdcparse
  libblifparse
  libeasygl
  libtatum
  libargparse
  libpugixml)

add_executable(genhlc src/main.cpp)
target_link_libraries(genhlc hlc)

#Specify link-time dependancies
install(TARGETS genhlc DESTINATION bin)

#
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_hlc ${TEST_SOURCES})
target_link_libraries(test_hlc hlc libcatch)

add_test(NAME test_hlc COMMAND test_hlc --use-colour=yes)
