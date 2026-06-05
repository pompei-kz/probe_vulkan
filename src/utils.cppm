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
