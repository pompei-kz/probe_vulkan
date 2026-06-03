// ReSharper disable CppUseStructuredBinding
module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

module triangle_application;

import utils;

namespace
{

  constexpr int WINDOW_WIDTH         = 800;
  constexpr int WINDOW_HEIGHT        = 600;
  constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  // ReSharper disable once CppTemplateArgumentsCanBeDeduced
  // ReSharper disable once CppVariableCanBeMadeConstexpr
  const std::vector<const char *> kDeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool complete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
  };

  struct SwapChainSupport
  {
    // Храним ограничения поверхности Vulkan для выбора параметров swap-chain.
    VkSurfaceCapabilitiesKHR capabilities{};
    // Доступные форматы поверхности Vulkan.
    std::vector<VkSurfaceFormatKHR> formats;
    // Доступные режимы показа Vulkan.
    std::vector<VkPresentModeKHR> presentModes;
  };

  std::vector<char> readFile(const std::filesystem::path &path)
  {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
      throw std::runtime_error("kQw7nPz4Lm :: failed to open " + path.string());
    }

    const size_t      fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
  }

} // namespace

struct TriangleApplication::Impl
{
  SDL_Window *window_ = nullptr;

  // Дескриптор экземпляра Vulkan.
  VkInstance instance_             = VK_NULL_HANDLE;
  // Дескриптор поверхности Vulkan для окна.
  VkSurfaceKHR surface_            = VK_NULL_HANDLE;
  // Выбранное физическое устройство Vulkan.
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  // Логическое устройство Vulkan.
  VkDevice device_                 = VK_NULL_HANDLE;

  // Очередь Vulkan для графических команд.
  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  // Очередь Vulkan для показа изображений.
  VkQueue presentQueue_  = VK_NULL_HANDLE;

  // Дескриптор цепочки обмена Vulkan.
  VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
  // Изображения swap-chain Vulkan.
  std::vector<VkImage> swapChainImages_;
  // Формат изображений swap-chain Vulkan.
  VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
  // Размер изображений swap-chain Vulkan.
  VkExtent2D swapChainExtent_{};
  // Image views Vulkan для изображений swap-chain.
  std::vector<VkImageView> swapChainImageViews_;
  // Framebuffers Vulkan для render pass.
  std::vector<VkFramebuffer> swapChainFramebuffers_;

  // Render pass Vulkan для отрисовки кадра.
  VkRenderPass renderPass_         = VK_NULL_HANDLE;
  // Layout графического pipeline Vulkan.
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  // Графический pipeline Vulkan.
  VkPipeline graphicsPipeline_     = VK_NULL_HANDLE;

  // Пул командных буферов Vulkan.
  VkCommandPool commandPool_ = VK_NULL_HANDLE;
  // Командные буферы Vulkan.
  std::vector<VkCommandBuffer> commandBuffers_;

  // Semaphores Vulkan для доступности изображений.
  std::vector<VkSemaphore> imageAvailableSemaphores_;
  // Semaphores Vulkan для завершения рендера.
  std::vector<VkSemaphore> renderFinishedSemaphores_;
  // Fences Vulkan для кадров в полете.
  std::vector<VkFence> inFlightFences_;
  uint32_t             currentFrame_       = 0;
  bool                 framebufferResized_ = false;

  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

  void initWindow()
  {
    // Инициализируем видео-подсистему SDL.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("pR8mX2vNaQ :: ") + SDL_GetError());
    }

    // Создаем окно SDL, совместимое с Vulkan.
    window_ = SDL_CreateWindow("Vulkan Triangle", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr)
    {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("aT5sJ9qBvE :: ") + SDL_GetError());
    }
  }

  void initVulkan()
  {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
  }

  void mainLoop()
  {
    bool quit = false;
    while (!quit)
    {
      SDL_Event event{};
      // Забираем следующее событие из очереди SDL.
      while (SDL_PollEvent(&event) != 0)
      {
        if (event.type == SDL_EVENT_QUIT)
        {
          quit = true;
        }
        else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
        {
          framebufferResized_ = true;
        }
      }

      drawFrame();
    }

    // Ждем завершения работы устройства Vulkan перед выходом из цикла.
    vkDeviceWaitIdle(device_);
  }

  void cleanup()
  {
    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      // Уничтожаем Vulkan semaphore ожидания завершения рендера.
      vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
      // Уничтожаем Vulkan semaphore доступности изображения.
      vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
      // Уничтожаем Vulkan fence кадра.
      vkDestroyFence(device_, inFlightFences_[i], nullptr);
    }

    // Уничтожаем пул команд Vulkan.
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    // Уничтожаем логическое устройство Vulkan.
    vkDestroyDevice(device_, nullptr);
    // Уничтожаем Vulkan surface через SDL.
    SDL_Vulkan_DestroySurface(instance_, surface_, nullptr);
    // Уничтожаем экземпляр Vulkan.
    vkDestroyInstance(instance_, nullptr);

    // Уничтожаем окно SDL.
    SDL_DestroyWindow(window_);
    // Завершаем работу SDL.
    SDL_Quit();
  }

  void createInstance()
  {
    // Описание приложения для создания экземпляра Vulkan.
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    Uint32 extensionCount         = 0;
    // Получаем список расширений Vulkan, которые нужны SDL.
    const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (extensions == nullptr)
    {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("hM4cV7nLxS :: ") + SDL_GetError());
    }

    // Параметры создания экземпляра Vulkan.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    // Создаем экземпляр Vulkan.
    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
    {
      throw std::runtime_error("uF2dK8wYpC :: failed to create Vulkan instance");
    }
  }

  void createSurface()
  {
    // Создаем Vulkan surface для окна SDL.
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_))
    {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("zN6rQ1tGmH :: ERROR IN `SDL_Vulkan_CreateSurface()`: ") + SDL_GetError());
    }
  }

  void pickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    // Запрашиваем количество физических устройств Vulkan.
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
      throw std::runtime_error("bL3xS9eRwD :: failed to find GPUs with Vulkan support");
    }

    // Список физических устройств Vulkan.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    // Получаем список физических устройств Vulkan.
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
      if (isDeviceSuitable(device))
      {
        physicalDevice_ = device;
        break;
      }
    }

    if (physicalDevice_ == VK_NULL_HANDLE)
    {
      throw std::runtime_error("ZGNjrNDGyX :: failed to find a suitable GPU");
    }
  }

  bool isDeviceSuitable(const VkPhysicalDevice device) const
  {
    const QueueFamilyIndices indices             = findQueueFamilies(device);
    const bool               extensionsSupported = checkDeviceExtensionSupport(device);
    bool                     swapChainAdequate   = false;

    if (extensionsSupported)
    {
      const SwapChainSupport swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate                       = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.complete() && extensionsSupported && swapChainAdequate;
  }

  static bool checkDeviceExtensionSupport(const VkPhysicalDevice device)
  {
    uint32_t extensionCount = 0;
    // Запрашиваем количество расширений устройства Vulkan.
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // Список доступных расширений устройства Vulkan.
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    // Получаем список расширений устройства Vulkan.
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
    // ReSharper disable once CppUseStructuredBinding
    for (const VkExtensionProperties &extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device) const
  {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    // Запрашиваем количество семейств очередей Vulkan.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Свойства семейств очередей Vulkan.
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    // Получаем свойства семейств очередей Vulkan.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
      if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
      {
        indices.graphicsFamily = i;
      }

      // Флаг поддержки показа через Vulkan surface.
      VkBool32 presentSupport = VK_FALSE;
      // Проверяем поддержку показа Vulkan для семейства очередей.
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
      if (presentSupport == VK_TRUE)
      {
        indices.presentFamily = i;
      }

      if (indices.complete())
      {
        break;
      }
    }

    return indices;
  }

  void createLogicalDevice()
  {
    // ReSharper disable once CppUseStructuredBinding
    const QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

    // Параметры очередей Vulkan для логического устройства.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // ReSharper disable once CppTemplateArgumentsCanBeDeduced
    std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily};

    constexpr float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      // Описание очереди Vulkan для логического устройства.
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    // Набор включаемых возможностей физического устройства Vulkan.
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Параметры создания логического устройства Vulkan.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

    // Создаем логическое устройство Vulkan.
    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS)
    {
      throw std::runtime_error("cY7pD4nVaR :: failed to create logical device");
    }

    // Получаем графическую очередь Vulkan.
    vkGetDeviceQueue(device_, *indices.graphicsFamily, 0, &graphicsQueue_);
    // Получаем очередь показа Vulkan.
    vkGetDeviceQueue(device_, *indices.presentFamily, 0, &presentQueue_);
  }

  SwapChainSupport querySwapChainSupport(const VkPhysicalDevice device) const
  {
    SwapChainSupport details;
    // Получаем ограничения Vulkan surface для swap-chain.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount = 0;
    // Запрашиваем количество форматов Vulkan surface.
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if (formatCount != 0)
    {
      details.formats.resize(formatCount);
      // Получаем доступные форматы Vulkan surface.
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    // Запрашиваем количество режимов показа Vulkan surface.
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
      details.presentModes.resize(presentModeCount);
      // Получаем доступные режимы показа Vulkan surface.
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
  {
    for (const auto &availableFormat : formats)
    {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return availableFormat;
      }
    }

    return formats[0];
  }

  static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes)
  {
    for (const auto &availablePresentMode : presentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return availablePresentMode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }

    int width  = 0;
    int height = 0;
    // Получаем размер окна SDL в пикселях.
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height))
    {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("mE9tH2wKsL :: ") + SDL_GetError());
    }

    // Фактический размер swap-chain Vulkan.
    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }

  void createSwapChain()
  {
    const SwapChainSupport swapChainSupport = querySwapChainSupport(physicalDevice_);
    // Выбранный формат поверхности Vulkan.
    const VkSurfaceFormatKHR surfaceFormat  = chooseSwapSurfaceFormat(swapChainSupport.formats);
    // Выбранный режим показа Vulkan.
    const VkPresentModeKHR presentMode      = chooseSwapPresentMode(swapChainSupport.presentModes);
    // Выбранный размер swap-chain Vulkan.
    const VkExtent2D extent                 = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t         imageCount             = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Параметры создания swap-chain Vulkan.
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface_;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const QueueFamilyIndices indices              = findQueueFamilies(physicalDevice_);
    const uint32_t           queueFamilyIndices[] = {*indices.graphicsFamily, *indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    // Создаем swap-chain Vulkan.
    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS)
    {
      throw std::runtime_error("xV5qN8cTpJ :: failed to create swap chain");
    }

    // Запрашиваем количество изображений swap-chain Vulkan.
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    // Получаем изображения swap-chain Vulkan.
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_      = extent;
  }

  void createImageViews()
  {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t i = 0; i < swapChainImages_.size(); ++i)
    {
      // Параметры создания image view Vulkan.
      VkImageViewCreateInfo createInfo{};
      createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image                           = swapChainImages_[i];
      createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format                          = swapChainImageFormat_;
      createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel   = 0;
      createInfo.subresourceRange.levelCount     = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount     = 1;

      // Создаем image view Vulkan для изображения swap-chain.
      if (vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("rB1mF6zQeW :: failed to create image views");
      }
    }
  }

  void createRenderPass()
  {
    // Описание цветового attachment Vulkan.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat_;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Ссылка на цветовой attachment Vulkan.
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Описание subpass Vulkan.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    // Зависимость subpass Vulkan для синхронизации.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Параметры создания render pass Vulkan.
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    // Создаем render pass Vulkan.
    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS)
    {
      throw std::runtime_error("nK8sP3yLdM :: failed to create render pass");
    }
  }

  [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char> &code) const
  {
    // Параметры создания shader module Vulkan.
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

    // Дескриптор shader module Vulkan.
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    // Создаем shader module Vulkan.
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("gW4vC9hTxN :: failed to create shader module");
    }
    return shaderModule;
  }

  void createGraphicsPipeline()
  {
    const std::filesystem::path basePath       = executableBasePath();
    const std::vector<char>     vertShaderCode = readFile(basePath / "shaders" / "triangle.vert.spv");
    const std::vector<char>     fragShaderCode = readFile(basePath / "shaders" / "triangle.frag.spv");

    // Vertex shader module Vulkan.
    const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    // Fragment shader module Vulkan.
    const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // Стадия vertex shader Vulkan.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    // Стадия fragment shader Vulkan.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    // Список shader stages Vulkan.
    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Описание входных вершин Vulkan.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Описание сборки примитивов Vulkan.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport Vulkan для области отрисовки.
    VkViewport viewport{};
    viewport.x        = 0.0F;
    viewport.y        = 0.0F;
    viewport.width    = static_cast<float>(swapChainExtent_.width);
    viewport.height   = static_cast<float>(swapChainExtent_.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    // Scissor Vulkan для ограничения области отрисовки.
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;

    // Состояние viewport/scissor Vulkan.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // Состояние растеризации Vulkan.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0F;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // Состояние multisampling Vulkan.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Настройки color blending Vulkan для attachment.
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    // Состояние color blending Vulkan.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // Layout pipeline Vulkan.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Создаем layout pipeline Vulkan.
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
    {
      throw std::runtime_error("dP6kR2mZaS :: failed to create pipeline layout");
    }

    // Параметры создания графического pipeline Vulkan.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.layout              = pipelineLayout_;
    pipelineInfo.renderPass          = renderPass_;
    pipelineInfo.subpass             = 0;

    // Создаем графический pipeline Vulkan.
    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS)
    {
      throw std::runtime_error("yL9fV5qBnE :: failed to create graphics pipeline");
    }

    // Уничтожаем fragment shader module Vulkan.
    vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    // Уничтожаем vertex shader module Vulkan.
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
  }

  void createFramebuffers()
  {
    swapChainFramebuffers_.resize(swapChainImageViews_.size());

    for (size_t i = 0; i < swapChainImageViews_.size(); ++i)
    {
      // Attachment framebuffer Vulkan.
      const VkImageView attachments[] = {swapChainImageViews_[i]};

      // Параметры создания framebuffer Vulkan.
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = renderPass_;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments    = attachments;
      framebufferInfo.width           = swapChainExtent_.width;
      framebufferInfo.height          = swapChainExtent_.height;
      framebufferInfo.layers          = 1;

      // Создаем framebuffer Vulkan.
      if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("tH3wX8cJpQ :: failed to create framebuffer");
      }
    }
  }

  void createCommandPool()
  {
    const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice_);

    // Параметры создания command pool Vulkan.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = *queueFamilyIndices.graphicsFamily;

    // Создаем command pool Vulkan.
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
    {
      throw std::runtime_error("sM7nD1vKgR :: failed to create command pool");
    }
  }

  void createCommandBuffers()
  {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    // Параметры выделения command buffers Vulkan.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool_;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    // Выделяем command buffers Vulkan.
    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
    {
      throw std::runtime_error("vC2pL9yWtF :: failed to allocate command buffers");
    }
  }

  void recordCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex) const
  {
    // Параметры начала записи command buffer Vulkan.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Начинаем запись command buffer Vulkan.
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("qN5eZ8rHsB :: failed to begin recording command buffer");
    }

    // Параметры начала render pass Vulkan.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass_;
    renderPassInfo.framebuffer       = swapChainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;

    // Цвет очистки Vulkan.
    constexpr VkClearValue clearColor = {{{0.02F, 0.03F, 0.05F, 1.0F}}};
    renderPassInfo.clearValueCount    = 1;
    renderPassInfo.pClearValues       = &clearColor;

    // Начинаем render pass Vulkan.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // Привязываем графический pipeline Vulkan.
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    // Отправляем команду рисования Vulkan.
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    // Завершаем render pass Vulkan.
    vkCmdEndRenderPass(commandBuffer);

    // Завершаем запись command buffer Vulkan.
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("wJ4tK6mPxV :: failed to record command buffer");
    }
  }

  void createSyncObjects()
  {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    // Параметры создания semaphore Vulkan.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Параметры создания fence Vulkan.
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      // Создаем semaphore Vulkan для ожидания изображения.
      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("eR8sB2qNyT :: failed to create image-available semaphore");
      }

      // Создаем semaphore Vulkan для ожидания завершения рендера.
      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("lD5vH9cWmK :: failed to create render-finished semaphore");
      }

      // Создаем fence Vulkan для кадра.
      if (vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("pX1kT7zQaF :: failed to create in-flight fence");
      }
    }
  }

  void drawFrame()
  {
    // Ждем fence Vulkan текущего кадра.
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    // Получаем следующее изображение swap-chain Vulkan.
    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      recreateSwapChain();
      return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      throw std::runtime_error("cM6yV3nLdP :: failed to acquire swap chain image");
    }

    // Сбрасываем fence Vulkan текущего кадра.
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
    // Сбрасываем command buffer Vulkan текущего кадра.
    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    // Semaphore Vulkan, которого ждет отправка команд.
    const VkSemaphore waitSemaphores[]          = {imageAvailableSemaphores_[currentFrame_]};
    // Стадия pipeline Vulkan, на которой ждем изображение.
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // Semaphore Vulkan, который сигнализируется после рендера.
    const VkSemaphore signalSemaphores[]        = {renderFinishedSemaphores_[currentFrame_]};

    // Параметры отправки команд Vulkan в очередь.
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    // Отправляем command buffer Vulkan в графическую очередь.
    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS)
    {
      throw std::runtime_error("aW9qE4sVrN :: failed to submit draw command buffer");
    }

    // Параметры показа изображения Vulkan.
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapChain_;
    presentInfo.pImageIndices      = &imageIndex;

    // Показываем изображение swap-chain Vulkan.
    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_)
    {
      framebufferResized_ = false;
      recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("hT2pK8mJxC :: failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void recreateSwapChain()
  {
    int width  = 0;
    int height = 0;
    // Получаем размер окна SDL в пикселях.
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    while (width == 0 || height == 0)
    {
      // Ждем событие SDL, пока окно свернуто или имеет нулевой размер.
      SDL_WaitEvent(nullptr);
      // Повторно получаем размер окна SDL в пикселях.
      SDL_GetWindowSizeInPixels(window_, &width, &height);
    }

    // Ждем завершения операций устройства Vulkan перед пересозданием swap-chain.
    vkDeviceWaitIdle(device_);
    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
  }

  void cleanupSwapChain()
  {
    for (const VkFramebuffer framebuffer : swapChainFramebuffers_)
    {
      // Уничтожаем framebuffer Vulkan.
      vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    swapChainFramebuffers_.clear();

    // Уничтожаем graphics pipeline Vulkan.
    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    graphicsPipeline_ = VK_NULL_HANDLE;

    // Уничтожаем layout pipeline Vulkan.
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    pipelineLayout_ = VK_NULL_HANDLE;

    // Уничтожаем render pass Vulkan.
    vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;

    for (const VkImageView imageView : swapChainImageViews_)
    {
      // Уничтожаем image view Vulkan.
      vkDestroyImageView(device_, imageView, nullptr);
    }
    swapChainImageViews_.clear();

    // Уничтожаем swap-chain Vulkan.
    vkDestroySwapchainKHR(device_, swapChain_, nullptr);
    swapChain_ = VK_NULL_HANDLE;
  }
};

TriangleApplication::TriangleApplication()
    : impl_(new Impl)
{
}

TriangleApplication::~TriangleApplication()
{
  delete impl_;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TriangleApplication::run()
{
  impl_->run();
}
