cmake_minimum_required(VERSION 4.2)
project(probe_vulkan)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SHADERC REQUIRED IMPORTED_TARGET shaderc)

add_executable(probe_vulkan src/main.cpp src/TriangleApplication.cpp)
target_sources(
  probe_vulkan
  PRIVATE
  FILE_SET CXX_MODULES
  FILES
  src/TriangleApplication.cppm
  src/utils.cppm
)

add_custom_command(
  TARGET probe_vulkan
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/files
    $<TARGET_FILE_DIR:probe_vulkan>
  VERBATIM
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 23)
  target_compile_options(probe_vulkan PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++2c>)
endif()

target_link_libraries(
  probe_vulkan
  PRIVATE
  Vulkan::Vulkan
  SDL3::SDL3
  PkgConfig::SHADERC
)
