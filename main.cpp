#include <vulkan/vulkan.h>
#include <iostream>

int main() {

  const auto lang = "C++";
  std::cout << "wKiQrZGKyD :: Hello and welcome to " << lang << "!\n";

  for (int i = 1; i <= 5; i++) {
    std::cout << "yysPrJOsq4 :: i = " << i << std::endl;
  }

  uint32_t version = VK_API_VERSION_1_3;

  std::cout
      << VK_API_VERSION_MAJOR(version)
      << "."
      << VK_API_VERSION_MINOR(version)
      << "."
      << VK_API_VERSION_PATCH(version)
      << std::endl;

  return 0;
}
