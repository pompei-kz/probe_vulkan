module;

#include <SDL3/SDL.h>
#include <chrono>
#include <filesystem>

export module utils;

export std::filesystem::path executableBasePath()
{
  // Получаем путь к каталогу исполняемого файла через SDL.
  const char *basePath = SDL_GetBasePath();

  if (basePath == nullptr) return {};

  return {basePath};
}

export std::string nowStr()
{
  const std::chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::now();

  const std::chrono::zoned_time tz{"Asia/Almaty", tp};

  const auto localTp = tz.get_local_time();

  const std::string time = std::format("{:%Y-%m-%d %H:%M:%S}.{:06}",
                                       std::chrono::floor<std::chrono::seconds>(localTp),
                                       std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count() % 1000000);

  return time;
}