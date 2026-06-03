cmake_minimum_required(VERSION 4.2)
project(probe_vulkan)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

set(SHADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag
)

set(SHADER_OUTPUTS)
foreach(SHADER_SOURCE ${SHADER_SOURCES})
  get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
  set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
  add_custom_command(
    OUTPUT ${SHADER_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
    DEPENDS ${SHADER_SOURCE}
    VERBATIM
  )
  list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(compile_shaders DEPENDS ${SHADER_OUTPUTS})

add_executable(probe_vulkan main.cpp TriangleApplication.cpp)
target_sources(
  probe_vulkan
  PRIVATE
  FILE_SET CXX_MODULES
  FILES
  TriangleApplication.cppm
)
add_dependencies(probe_vulkan compile_shaders)

target_link_libraries(
  probe_vulkan
  PRIVATE
  Vulkan::Vulkan
  SDL3::SDL3
)
