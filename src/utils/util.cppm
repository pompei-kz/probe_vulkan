module;

#include <SDL3/SDL.h>
#include <shaderc/shaderc.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

export module util;

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

} // namespace util
