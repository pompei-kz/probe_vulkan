module;

#include <SDL3/SDL.h>

#include <filesystem>

export module utils;

export std::filesystem::path executableBasePath()
{
  // Получаем путь к каталогу исполняемого файла через SDL.
  const char *basePath = SDL_GetBasePath();

  if (basePath == nullptr) return {};

  return {basePath};
}
