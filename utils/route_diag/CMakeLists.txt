cmake_minimum_required(VERSION 3.16)
cmake_policy(VERSION 3.10)

project("route_diag")

add_executable(route_diag src/main.cpp)
target_link_libraries(route_diag
  libvpr
  )

