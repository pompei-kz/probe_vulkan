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
  src/Context.cppm
  src/Bean01.cppm
  src/Bean02.cppm
  src/Bean03.cppm
  src/Bean04.cppm
  src/getter.cppm
  src/TriangleApplication.cppm
  src/utils.cppm
)

file(GLOB_RECURSE RUNTIME_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/files/*)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_files.stamp
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/files
    $<TARGET_FILE_DIR:probe_vulkan>
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_files.stamp
  DEPENDS ${RUNTIME_FILES}
  VERBATIM
)

add_custom_target(copy_runtime_files DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_files.stamp)
add_dependencies(probe_vulkan copy_runtime_files)

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
