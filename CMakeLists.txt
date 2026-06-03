cmake_minimum_required(VERSION 4.2)
project(probe_vulkan)

set(CMAKE_CXX_STANDARD 26)

add_executable(probe_vulkan main.cpp)

find_package(Vulkan REQUIRED)

target_link_libraries(
  probe_vulkan
  PRIVATE
  Vulkan::Vulkan
)
