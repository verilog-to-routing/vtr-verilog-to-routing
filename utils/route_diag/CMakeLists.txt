cmake_minimum_required(VERSION 3.9)
cmake_policy(VERSION 3.9)

project("route_diag")

add_executable(route_diag src/main.cpp)
target_link_libraries(route_diag
  libvpr
  libvtrutil
  libarchfpga
  libsdcparse
  libblifparse
  libeasygl
  libtatum
  libargparse
  libpugixml)
