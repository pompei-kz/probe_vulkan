module;

#include <SDL3/SDL.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shaderc/shaderc.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

export module util;

namespace {
  void printVkQueueFlag0(bool                   &first, //
                         const std::string_view &prefix,
                         const std::string      &spaces,
                         const VkQueueFlags      queueFlags,
                         const VkQueueFlagBits   flagBit,
                         const char             *display)
  {
    if (queueFlags & flagBit) {
      std::cout << (first ? prefix : spaces) << display << std::endl;
      first = false;
    }
  }

  void printVkQueueFlags0(const std::string_view &prefix, const VkQueueFlags queueFlags)
  {
    const std::string spaces(prefix.length(), ' ');

    bool first = true;

    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_GRAPHICS_BIT, "VK_QUEUE_GRAPHICS_BIT");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_COMPUTE_BIT, "VK_QUEUE_COMPUTE_BIT");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_TRANSFER_BIT, "VK_QUEUE_TRANSFER_BIT");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_SPARSE_BINDING_BIT, "VK_QUEUE_SPARSE_BINDING_BIT");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_PROTECTED_BIT, "VK_QUEUE_PROTECTED_BIT");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_VIDEO_DECODE_BIT_KHR, "VK_QUEUE_VIDEO_DECODE_BIT_KHR");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_VIDEO_ENCODE_BIT_KHR, "VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
    printVkQueueFlag0(first, prefix, spaces, queueFlags, VK_QUEUE_OPTICAL_FLOW_BIT_NV, "VK_QUEUE_OPTICAL_FLOW_BIT_NV");

    if (first) {
      std::cout << prefix << "(NO BITS)" << std::endl;
    }
  }
} // namespace

export namespace util {

  std::filesystem::path executableBasePath()
  {
    // Получаем путь к каталогу исполняемого файла через SDL.
    const char *basePath = SDL_GetBasePath();

    if (basePath == nullptr) return {};

    return {basePath};
  }

  std::string readTextFile(const std::filesystem::path &path)
  {
    std::ifstream file(path, std::ios::ate);

    if (!file.is_open()) {
      throw std::runtime_error("kQw7nPz4Lm :: failed to open " + path.string());
    }

    const size_t fileSize = file.tellg();
    std::string  buffer(fileSize, '\0');
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    return buffer;
  }

  std::vector<uint32_t> compileShader(const std::filesystem::path &path, const shaderc_shader_kind shaderKind)
  {
    const std::string source     = readTextFile(path);
    const std::string pathString = path.string();

    shaderc::Compiler                   compiler;
    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, shaderKind, pathString.c_str());

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
      throw std::runtime_error("tH7qN4vZpR :: failed to compile shader " + pathString + ": " + result.GetErrorMessage());
    }

    return {result.cbegin(), result.cend()};
  }

  std::string nowStr()
  {
    const auto                           tp = std::chrono::system_clock::now();
    const std::chrono::time_zone *const  tz = std::chrono::current_zone();
    const std::chrono::zoned_time        zt{tz, tp};
    const auto                           localTp    = zt.get_local_time();
    const auto                           time_point = std::chrono::floor<std::chrono::seconds>(localTp);
    const std::chrono::microseconds      eid        = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch());
    const std::chrono::microseconds::rep count      = eid.count() % 1000000;
    const std::string                    time       = std::format("{:%Y-%m-%d %H:%M:%S}.{:06}", time_point, count);

    return time;
  }

  void printVkQueueFlags(std::string_view prefix, VkQueueFlags queueFlags)
  {
    printVkQueueFlags0(prefix, queueFlags);
  }

  std::string VkExtent3D_to_str(VkExtent3D value)
  {
    return std::format(
        "VkExtent3D{{{}x{}, depth={}}}",
        value.width,
        value.height,
        value.depth
    );
  }

  const char* VkResultToString(VkResult r)
  {
    switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    default: return "UNKNOWN";
    }
  }

} // namespace util
