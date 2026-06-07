// ReSharper disable CppUseStructuredBinding
module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

export module triangle_application_vulkan_init;

import util;
import cmd_pipeline;
import vulkanPipeline;
import model;

namespace app {

  constexpr int WINDOW_WIDTH  = 800;
  constexpr int WINDOW_HEIGHT = 600;

  // Количество кадров, которые CPU может подготавливать одновременно.
  constexpr int MAX_FRAMES_IN_FLIGHT = model::MAX_FRAMES_IN_FLIGHT;

  // ReSharper disable once CppTemplateArgumentsCanBeDeduced
  // ReSharper disable once CppVariableCanBeMadeConstexpr
  const std::vector<const char *> kDeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  export class VulkanInit
  {
  public:
    void setWindow(SDL_Window *window);
    void setCameraPosition(glm::vec3 position);
    void setCameraForward(glm::vec3 forward);
    void setCameraUp(glm::vec3 up);
    void setCameraPlanes(float nearPlane, float farPlane);
    void setCameraFovDegrees(float fovDegrees);

    // Загружает новый набор источников света. Данные будут переписаны во все слоты кольца.
    void setLights(std::vector<model::LightGpu> lights);

    // Создает все статические Vulkan ресурсы pipeline (один раз при регистрации).
    void createPipeline(model::PipelineVk_ShapeGroup &pipeline);
    // Уничтожает все Vulkan ресурсы pipeline (при удалении или замене).
    void destroyPipeline(model::PipelineVk_ShapeGroup &pipeline) const;

    void waitIdle() const;
    void cleanup();
    void drawFrame(const std::vector<model::PipelineVk_ShapeGroup *> &pipelines, bool &framebufferResized);
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayouts();
    void createSharedPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createLightResources();
    void createCommandBuffers();
    void createSyncObjects();

  private:
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
    VkRenderPass renderPass_ = VK_NULL_HANDLE;

    // Общие descriptor set layouts (живут все время работы устройства).
    VkDescriptorSetLayout globalSetLayout_   = VK_NULL_HANDLE; // set 0: свет
    VkDescriptorSetLayout materialSetLayout_ = VK_NULL_HANDLE; // set 1: материалы

    // Общий графический pipeline (одинаковые шейдеры и формат вершин для всех pipeline).
    VkPipelineLayout sharedPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline       sharedPipeline_       = VK_NULL_HANDLE;

    // Ресурсы света: пул, по одному descriptor set и буферу на кадр в полете.
    VkDescriptorPool                                  lightPool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> lightSets_{};
    std::array<model::RingSlot, MAX_FRAMES_IN_FLIGHT> lightRing_{};
    std::vector<model::LightGpu>                      lightData_;
    glm::vec4                                         lightAmbient_{0.18F, 0.18F, 0.18F, 0.0F};
    int                                               lightUploadsRemaining_ = 0;

    // Пул командных буферов Vulkan.
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    // Командные буферы Vulkan.
    std::vector<VkCommandBuffer> commandBuffers_;

    // Semaphores Vulkan для доступности изображений.
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    // Semaphores Vulkan для завершения рендера.
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    // Fences Vulkan для кадров в полете.
    std::vector<VkFence>     inFlightFences_;
    uint32_t                 currentFrame_ = 0;
    glm::vec3                cameraPosition_{0.0F, 0.0F, 2.0F};
    glm::vec3                cameraForward_{0.0F, 0.0F, -1.0F};
    glm::vec3                cameraUp_{0.0F, 1.0F, 0.0F};
    float                    cameraNearPlane_  = 0.1F;
    float                    cameraFarPlane_   = 10.0F;
    float                    cameraFovDegrees_ = 45.0F;
    model::TransformMatrices transforms_{
        .model      = glm::mat4(1.0F),
        .view       = glm::lookAt(glm::vec3(0.0F, 0.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)),
        .projection = glm::perspective(glm::radians(45.0F), static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT), 0.1F, 10.0F),
    };
    model::PushConstants pushConstants_{};

    bool                      isDeviceSuitable(const VkPhysicalDevice device) const;
    static bool               checkDeviceExtensionSupport(const VkPhysicalDevice device);
    model::QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device) const;
    model::SwapChainSupport   querySwapChainSupport(const VkPhysicalDevice device) const;
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
    static VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes);
    [[nodiscard]] VkExtent2D  chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;
    void                      updateTransformMatrices();

    void updateLightDescriptor(uint32_t frame) const;
    void writeLightSlot(uint32_t frame);

    void recordCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, const std::vector<model::PipelineRenderData> &renderDatas) const;
    void recreateSwapChain();
    void cleanupSwapChain();
  };

} // namespace app

namespace app {

  void VulkanInit::setWindow(SDL_Window *window)
  {
    window_ = window;
  }

  void VulkanInit::setCameraPosition(const glm::vec3 position)
  {
    cameraPosition_ = position;
    updateTransformMatrices();
  }

  void VulkanInit::setCameraForward(const glm::vec3 forward)
  {
    if (glm::dot(forward, forward) <= 0.0F) {
      throw std::invalid_argument("fV3sW9nAeQ :: camera forward vector must be non-zero");
    }
    cameraForward_ = glm::normalize(forward);
    updateTransformMatrices();
  }

  void VulkanInit::setCameraUp(const glm::vec3 up)
  {
    if (glm::dot(up, up) <= 0.0F) {
      throw std::invalid_argument("qJ8mR2cLpD :: camera up vector must be non-zero");
    }

    const glm::vec3 forward  = glm::normalize(cameraForward_);
    const glm::vec3 planarUp = up - glm::dot(up, forward) * forward;
    if (glm::dot(planarUp, planarUp) <= 0.0F) {
      throw std::invalid_argument("nY6kT4vBxH :: camera up vector must not be parallel to forward vector");
    }

    cameraUp_ = glm::normalize(planarUp);
    updateTransformMatrices();
  }

  void VulkanInit::setCameraPlanes(const float nearPlane, const float farPlane)
  {
    if (nearPlane <= 0.0F || farPlane <= nearPlane) {
      throw std::invalid_argument("zD9pH5wKuM :: invalid camera clipping planes");
    }
    cameraNearPlane_ = nearPlane;
    cameraFarPlane_  = farPlane;
    updateTransformMatrices();
  }

  void VulkanInit::setCameraFovDegrees(const float fovDegrees)
  {
    if (fovDegrees <= 0.0F || fovDegrees >= 180.0F) {
      throw std::invalid_argument("pC2rL7xNsV :: camera fovDegrees must be between 0 and 180");
    }
    cameraFovDegrees_ = fovDegrees;
    updateTransformMatrices();
  }

  void VulkanInit::setLights(std::vector<model::LightGpu> lights)
  {
    lightData_             = std::move(lights);
    // Перезаписать данные нужно во всех слотах кольца, поэтому помечаем все кадры.
    lightUploadsRemaining_ = MAX_FRAMES_IN_FLIGHT;
  }

  void VulkanInit::createPipeline(model::PipelineVk_ShapeGroup &pipeline)
  {
    vulkan_pipeline::createPipeline(device_, physicalDevice_, materialSetLayout_, pipeline);
  }

  void VulkanInit::destroyPipeline(model::PipelineVk_ShapeGroup &pipeline) const
  {
    vulkan_pipeline::destroyPipeline(device_, pipeline);
  }

  void VulkanInit::createInstance()
  {
    // Описание приложения для создания экземпляра Vulkan.
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Тип структуры с информацией о приложении.
    appInfo.pApplicationName   = "Vulkan Triangle";                  // Имя приложения для драйвера и отладочных инструментов.
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);           // Версия приложения.
    appInfo.pEngineName        = "No Engine";                        // Имя движка, если он используется.
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);           // Версия движка.
    appInfo.apiVersion         = VK_API_VERSION_1_0;                 // Минимальная версия Vulkan API для приложения.

    Uint32 extensionCount         = 0;
    // Получаем список расширений Vulkan, которые нужны SDL.
    const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (extensions == nullptr) {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("hM4cV7nLxS :: ") + SDL_GetError());
    }

    // Параметры создания экземпляра Vulkan.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Тип структуры создания экземпляра Vulkan.
    createInfo.pApplicationInfo        = &appInfo;                               // Указатель на описание приложения.
    createInfo.enabledExtensionCount   = extensionCount;                         // Количество включаемых расширений экземпляра.
    createInfo.ppEnabledExtensionNames = extensions;                             // Имена расширений экземпляра, нужных SDL.

    // Создаем экземпляр Vulkan.
    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
      throw std::runtime_error("uF2dK8wYpC :: failed to create Vulkan instance");
    }
  }

  void VulkanInit::createSurface()
  {
    // Создаем Vulkan surface для окна SDL.
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("zN6rQ1tGmH :: ERROR IN `SDL_Vulkan_CreateSurface()`: ") + SDL_GetError());
    }
  }

  void VulkanInit::pickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    // Запрашиваем количество физических устройств Vulkan.
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
      throw std::runtime_error("bL3xS9eRwD :: failed to find GPUs with Vulkan support");
    }

    // Список физических устройств Vulkan.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    // Получаем список физических устройств Vulkan.
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const VkPhysicalDevice &device : devices) {
      if (isDeviceSuitable(device)) {
        physicalDevice_ = device;
        break;
      }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
      throw std::runtime_error("ZGNjrNDGyX :: failed to find a suitable GPU");
    }
  }

  bool VulkanInit::isDeviceSuitable(const VkPhysicalDevice device) const
  {
    const model::QueueFamilyIndices indices             = findQueueFamilies(device);
    const bool                      extensionsSupported = checkDeviceExtensionSupport(device);
    bool                            swapChainAdequate   = false;

    if (extensionsSupported) {
      const model::SwapChainSupport swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate                              = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.complete() && extensionsSupported && swapChainAdequate;
  }

  bool VulkanInit::checkDeviceExtensionSupport(const VkPhysicalDevice device)
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
    for (const VkExtensionProperties &extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }

  model::QueueFamilyIndices VulkanInit::findQueueFamilies(const VkPhysicalDevice device) const
  {
    model::QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    // Запрашиваем количество семейств очередей Vulkan.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Свойства семейств очередей Vulkan.
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    // Получаем свойства семейств очередей Vulkan.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
      if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
        indices.graphicsFamily = i;
      }

      // Флаг поддержки показа через Vulkan surface.
      VkBool32 presentSupport = VK_FALSE;
      // Проверяем поддержку показа Vulkan для семейства очередей.
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
      if (presentSupport == VK_TRUE) {
        indices.presentFamily = i;
      }

      if (indices.complete()) {
        break;
      }
    }

    return indices;
  }

  void VulkanInit::createLogicalDevice()
  {
    // ReSharper disable once CppUseStructuredBinding
    const model::QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

    // Параметры очередей Vulkan для логического устройства.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // ReSharper disable once CppTemplateArgumentsCanBeDeduced
    std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily};

    constexpr float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
      // Описание очереди Vulkan для логического устройства.
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; // Тип структуры создания очереди устройства.
      queueCreateInfo.queueFamilyIndex = queueFamily;                                // Индекс семейства очередей, из которого берется очередь.
      queueCreateInfo.queueCount       = 1;                                          // Количество создаваемых очередей в этом семействе.
      queueCreateInfo.pQueuePriorities = &queuePriority;                             // Приоритет очереди для планировщика устройства.
      queueCreateInfos.push_back(queueCreateInfo);
    }

    // Набор включаемых возможностей физического устройства Vulkan.
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Параметры создания логического устройства Vulkan.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;            // Тип структуры создания логического устройства.
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());  // Количество описаний очередей устройства.
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();                         // Описания очередей, которые нужно создать.
    createInfo.pEnabledFeatures        = &deviceFeatures;                                 // Включаемые возможности физического устройства.
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size()); // Количество расширений устройства.
    createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();                        // Имена включаемых расширений устройства.

    // Создаем логическое устройство Vulkan.
    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
      throw std::runtime_error("cY7pD4nVaR :: failed to create logical device");
    }

    // Получаем графическую очередь Vulkan.
    vkGetDeviceQueue(device_, *indices.graphicsFamily, 0, &graphicsQueue_);
    // Получаем очередь показа Vulkan.
    vkGetDeviceQueue(device_, *indices.presentFamily, 0, &presentQueue_);
  }

  model::SwapChainSupport VulkanInit::querySwapChainSupport(const VkPhysicalDevice device) const
  {
    model::SwapChainSupport details;
    // Получаем ограничения Vulkan surface для swap-chain.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount = 0;
    // Запрашиваем количество форматов Vulkan surface.
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if (formatCount != 0) {
      details.formats.resize(formatCount);
      // Получаем доступные форматы Vulkan surface.
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    // Запрашиваем количество режимов показа Vulkan surface.
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      // Получаем доступные режимы показа Vulkan surface.
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
  }

  VkSurfaceFormatKHR VulkanInit::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
  {
    for (const auto &availableFormat : formats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }

    return formats[0];
  }

  VkPresentModeKHR VulkanInit::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes)
  {
    for (const auto &availablePresentMode : presentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  [[nodiscard]] VkExtent2D VulkanInit::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }

    int width  = 0;
    int height = 0;

    // Получаем размер окна SDL в пикселях.
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height)) {
      // Получаем текст ошибки SDL.
      throw std::runtime_error(std::string("mE9tH2wKsL :: ") + SDL_GetError());
    }

    // Фактический размер swap-chain Vulkan.
    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }

  void VulkanInit::updateTransformMatrices()
  {
    transforms_.model = glm::mat4(1.0F);
    transforms_.view  = glm::lookAt(cameraPosition_, cameraPosition_ + cameraForward_, cameraUp_);

    const float aspect     = static_cast<float>(swapChainExtent_.width) / static_cast<float>(swapChainExtent_.height);
    transforms_.projection = glm::perspective(glm::radians(cameraFovDegrees_), aspect, cameraNearPlane_, cameraFarPlane_);
    transforms_.projection[1][1] *= -1.0F;
    pushConstants_.view       = transforms_.view;
    pushConstants_.projection = transforms_.projection;
  }

  void VulkanInit::createSwapChain()
  {
    const model::SwapChainSupport swapChainSupport = querySwapChainSupport(physicalDevice_);
    // Выбранный формат поверхности Vulkan.
    const VkSurfaceFormatKHR surfaceFormat         = chooseSwapSurfaceFormat(swapChainSupport.formats);
    // Выбранный режим показа Vulkan.
    const VkPresentModeKHR presentMode             = chooseSwapPresentMode(swapChainSupport.presentModes);
    // Выбранный размер swap-chain Vulkan.
    const VkExtent2D extent                        = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t         imageCount                    = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Параметры создания swap-chain Vulkan.
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // Тип структуры создания swap-chain.
    createInfo.surface          = surface_;                                    // Surface окна, для которого создается swap-chain.
    createInfo.minImageCount    = imageCount;                                  // Минимальное количество изображений в swap-chain.
    createInfo.imageFormat      = surfaceFormat.format;                        // Формат пикселей изображений swap-chain.
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;                    // Цветовое пространство изображений swap-chain.
    createInfo.imageExtent      = extent;                                      // Размер изображений swap-chain в пикселях.
    createInfo.imageArrayLayers = 1;                                           // Количество слоев изображения, для обычного 2D окна нужен один.
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;         // Изображения будут использоваться как color attachment.

    const model::QueueFamilyIndices indices              = findQueueFamilies(physicalDevice_);
    const uint32_t                  queueFamilyIndices[] = {*indices.graphicsFamily, *indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT; // Изображения доступны нескольким семействам очередей.
      createInfo.queueFamilyIndexCount = 2;                          // Количество семейств очередей, которым нужен доступ.
      createInfo.pQueueFamilyIndices   = queueFamilyIndices;         // Индексы графического и present семейств очередей.
    } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Изображения принадлежат одному семейству очередей.
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform; // Текущее преобразование surface перед показом.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;              // Альфа-канал окна не смешивается с другими окнами.
    createInfo.presentMode    = presentMode;                                    // Режим показа изображений на экран.
    createInfo.clipped        = VK_TRUE;                                        // Разрешаем не рисовать скрытые части окна.
    createInfo.oldSwapchain   = VK_NULL_HANDLE;                                 // Старый swap-chain отсутствует при первом создании.

    // Создаем swap-chain Vulkan.
    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
      throw std::runtime_error("xV5qN8cTpJ :: failed to create swap chain");
    }

    // Запрашиваем количество изображений swap-chain Vulkan.
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    // Получаем изображения swap-chain Vulkan.
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_      = extent;
    updateTransformMatrices();
  }

  void VulkanInit::createImageViews()
  {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t i = 0; i < swapChainImages_.size(); ++i) {
      // Параметры создания image view Vulkan.
      VkImageViewCreateInfo createInfo{};
      createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; // Тип структуры создания image view.
      createInfo.image                           = swapChainImages_[i];                      // Изображение swap-chain, для которого создается view.
      createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;                    // Представление изображения как 2D texture.
      createInfo.format                          = swapChainImageFormat_;                    // Формат данных изображения.
      createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;            // Красный канал остается без перестановки.
      createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;            // Зеленый канал остается без перестановки.
      createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;            // Синий канал остается без перестановки.
      createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;            // Альфа-канал остается без перестановки.
      createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;                // View обращается к цветовой части изображения.
      createInfo.subresourceRange.baseMipLevel   = 0;                                        // Первый mip level для view.
      createInfo.subresourceRange.levelCount     = 1;                                        // Количество mip levels в view.
      createInfo.subresourceRange.baseArrayLayer = 0;                                        // Первый слой массива изображений.
      createInfo.subresourceRange.layerCount     = 1;                                        // Количество слоев изображения в view.

      // Создаем image view Vulkan для изображения swap-chain.
      if (vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS) {
        throw std::runtime_error("rB1mF6zQeW :: failed to create image views");
      }
    }
  }

  void VulkanInit::createRenderPass()
  {
    // Описание цветового attachment Vulkan.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat_;            // Формат color attachment совпадает с форматом swap-chain.
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;            // Multisampling отключен, используется один sample на пиксель.
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;      // Перед рендерингом attachment очищается clear color.
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;     // После рендеринга результат сохраняется для показа.
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // Stencil-данные не используются, их загрузка не важна.
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Stencil-данные не используются, их сохранение не важно.
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;        // Предыдущее содержимое изображения не нужно.
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // После render pass изображение готово к показу.

    // Ссылка на цветовой attachment Vulkan.
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;                                        // Индекс color attachment в массиве render pass attachments.
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout attachment во время цветового рендеринга.

    // Описание subpass Vulkan.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS; // Subpass используется графическим pipeline.
    subpass.colorAttachmentCount = 1;                               // В subpass используется один color attachment.
    subpass.pColorAttachments    = &colorAttachmentRef;             // Ссылка на color attachment для вывода fragment shader.

    // Зависимость subpass Vulkan для синхронизации.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;                           // Источник зависимости находится вне render pass.
    dependency.dstSubpass    = 0;                                             // Зависимость направлена в первый subpass.
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Ждем стадии вывода color attachment снаружи.
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Синхронизируемся перед стадией вывода color attachment.
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;          // Разрешаем запись в color attachment после ожидания.

    // Параметры создания render pass Vulkan.
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; // Тип структуры создания render pass.
    renderPassInfo.attachmentCount = 1;                                         // Количество attachments в render pass.
    renderPassInfo.pAttachments    = &colorAttachment;                          // Описание color attachment.
    renderPassInfo.subpassCount    = 1;                                         // Количество subpasses в render pass.
    renderPassInfo.pSubpasses      = &subpass;                                  // Описание subpass.
    renderPassInfo.dependencyCount = 1;                                         // Количество зависимостей subpass.
    renderPassInfo.pDependencies   = &dependency;                               // Описание синхронизации subpass.

    // Создаем render pass Vulkan.
    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
      throw std::runtime_error("nK8sP3yLdM :: failed to create render pass");
    }
  }

  void VulkanInit::createDescriptorSetLayouts()
  {
    vulkan_pipeline::createDescriptorSetLayouts(device_, globalSetLayout_, materialSetLayout_);
  }

  void VulkanInit::createSharedPipeline()
  {
    vulkan_pipeline::createGraphicsPipeline(device_,
                                            swapChainExtent_,
                                            renderPass_,
                                            globalSetLayout_,
                                            materialSetLayout_,
                                            sharedPipelineLayout_,
                                            sharedPipeline_);
  }

  void VulkanInit::createFramebuffers()
  {
    swapChainFramebuffers_.resize(swapChainImageViews_.size());

    for (size_t i = 0; i < swapChainImageViews_.size(); ++i) {
      // Attachment framebuffer Vulkan.
      const VkImageView attachments[] = {swapChainImageViews_[i]};

      // Параметры создания framebuffer Vulkan.
      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; // Тип структуры создания framebuffer.
      framebufferInfo.renderPass      = renderPass_;                               // Render pass, для которого создается framebuffer.
      framebufferInfo.attachmentCount = 1;                                         // Количество attachments framebuffer.
      framebufferInfo.pAttachments    = attachments;                               // Image view swap-chain как attachment framebuffer.
      framebufferInfo.width           = swapChainExtent_.width;                    // Ширина framebuffer.
      framebufferInfo.height          = swapChainExtent_.height;                   // Высота framebuffer.
      framebufferInfo.layers          = 1;                                         // Количество слоев framebuffer.

      // Создаем framebuffer Vulkan.
      if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS) {
        throw std::runtime_error("tH3wX8cJpQ :: failed to create framebuffer");
      }
    }
  }

  void VulkanInit::createCommandPool()
  {
    const model::QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice_);

    // Параметры создания command pool Vulkan.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;      // Тип структуры создания command pool.
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Разрешаем сбрасывать command buffers по отдельности.
    poolInfo.queueFamilyIndex = *queueFamilyIndices.graphicsFamily;              // Семейство очередей, для которого создается command pool.

    // Создаем command pool Vulkan.
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
      throw std::runtime_error("sM7nD1vKgR :: failed to create command pool");
    }
  }

  void VulkanInit::updateLightDescriptor(const uint32_t frame) const
  {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = lightRing_[frame].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = lightSets_[frame];
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo     = &bufferInfo;

    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
  }

  void VulkanInit::writeLightSlot(const uint32_t frame)
  {
    model::RingSlot &slot = lightRing_[frame];
    const uint32_t   need = static_cast<uint32_t>(lightData_.size());

    if (need > slot.capacity) {
      const uint32_t newCapacity = std::max(need, slot.capacity * 2);
      vulkan_pipeline::destroyRingSlot(device_, slot);
      vulkan_pipeline::createRingSlot(device_,
                                      physicalDevice_,
                                      slot,
                                      sizeof(model::LightBufferHeader) + sizeof(model::LightGpu) * newCapacity,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      newCapacity);
      updateLightDescriptor(frame);
    }

    auto *base = static_cast<uint8_t *>(slot.mapped);

    model::LightBufferHeader header{};
    header.count   = need;
    header.ambient = lightAmbient_;
    std::memcpy(base, &header, sizeof(header));

    if (need > 0) {
      std::memcpy(base + sizeof(model::LightBufferHeader), lightData_.data(), sizeof(model::LightGpu) * need);
    }
  }

  void VulkanInit::createLightResources()
  {
    // Пул, из которого выделяются descriptor sets света (по одному на кадр в полете).
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &lightPool_) != VK_SUCCESS) {
      throw std::runtime_error("dV8nR3mKpL :: failed to create light descriptor pool");
    }

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
    layouts.fill(globalSetLayout_);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = lightPool_;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(device_, &allocInfo, lightSets_.data()) != VK_SUCCESS) {
      throw std::runtime_error("kP2sT7mNwB :: failed to allocate light descriptor sets");
    }

    constexpr uint32_t initialLightCapacity = 8;
    for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
      vulkan_pipeline::createRingSlot(device_,
                                      physicalDevice_,
                                      lightRing_[frame],
                                      sizeof(model::LightBufferHeader) + sizeof(model::LightGpu) * initialLightCapacity,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      initialLightCapacity);
      updateLightDescriptor(frame);
      // Изначально источников света нет: пишем заголовок с count = 0.
      writeLightSlot(frame);
    }
  }

  void VulkanInit::createCommandBuffers()
  {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    // Параметры выделения command buffers Vulkan.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; // Тип структуры выделения command buffers.
    allocInfo.commandPool        = commandPool_;                                   // Command pool, из которого выделяются command buffers.
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;                // Primary command buffers можно отправлять в очередь напрямую.
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());  // Количество выделяемых command buffers.

    // Выделяем command buffers Vulkan.
    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
      throw std::runtime_error("vC2pL9yWtF :: failed to allocate command buffers");
    }
  }

  void VulkanInit::recordCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex, const std::vector<model::PipelineRenderData> &renderDatas) const
  {
    vulkan_pipeline::recordCommandBuffer(commandBuffer,
                                         imageIndex,
                                         renderPass_,
                                         swapChainFramebuffers_,
                                         swapChainExtent_,
                                         sharedPipeline_,
                                         sharedPipelineLayout_,
                                         lightSets_[currentFrame_],
                                         pushConstants_,
                                         renderDatas);
  }

  void VulkanInit::createSyncObjects()
  {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    // Параметры создания semaphore Vulkan.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO; // Тип структуры создания semaphore.

    // Параметры создания fence Vulkan.
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; // Тип структуры создания fence.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;        // Fence создается уже signaled, чтобы первый кадр не завис.

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      // Создаем semaphore Vulkan для ожидания изображения.
      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS) {
        throw std::runtime_error("eR8sB2qNyT :: failed to create image-available semaphore");
      }

      // Создаем semaphore Vulkan для ожидания завершения рендера.
      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS) {
        throw std::runtime_error("lD5vH9cWmK :: failed to create render-finished semaphore");
      }

      // Создаем fence Vulkan для кадра.
      if (vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
        throw std::runtime_error("pX1kT7zQaF :: failed to create in-flight fence");
      }
    }
  }

  void VulkanInit::waitIdle() const
  {
    // Ждем завершения работы устройства Vulkan перед выходом из цикла.
    vkDeviceWaitIdle(device_);
  }

  void VulkanInit::cleanup()
  {
    cleanupSwapChain();

    for (model::RingSlot &slot : lightRing_) {
      vulkan_pipeline::destroyRingSlot(device_, slot);
    }
    if (lightPool_ != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(device_, lightPool_, nullptr);
      lightPool_ = VK_NULL_HANDLE;
    }

    if (materialSetLayout_ != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device_, materialSetLayout_, nullptr);
      materialSetLayout_ = VK_NULL_HANDLE;
    }
    if (globalSetLayout_ != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device_, globalSetLayout_, nullptr);
      globalSetLayout_ = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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
  }

  void VulkanInit::drawFrame(const std::vector<model::PipelineVk_ShapeGroup *> &pipelines, bool &framebufferResized)
  {
    // Ждем fence Vulkan текущего кадра. После этого GPU точно закончил работу с буферами этого кадра.
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    // Получаем следующее изображение swap-chain Vulkan.
    VkResult result = vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("cM6yV3nLdP :: failed to acquire swap chain image");
    }

    // Сбрасываем fence Vulkan текущего кадра.
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    // Безопасно обновляем буферы текущего кадра (GPU их больше не читает).
    if (lightUploadsRemaining_ > 0) {
      writeLightSlot(currentFrame_);
      --lightUploadsRemaining_;
    }

    std::vector<model::PipelineRenderData> renderDatas;
    renderDatas.reserve(pipelines.size());
    for (model::PipelineVk_ShapeGroup *pipeline : pipelines) {
      if (pipeline == nullptr) continue;

      vulkan_pipeline::updateInstanceData(device_, physicalDevice_, *pipeline, currentFrame_);
      renderDatas.push_back(model::PipelineRenderData{
          .materialSet    = pipeline->gpu.materialSet,
          .vertexBuffer   = pipeline->gpu.vertexBuffer,
          .indexBuffer    = pipeline->gpu.indexBuffer,
          .instanceBuffer = pipeline->gpu.instanceRing[currentFrame_].buffer,
          .batches        = &pipeline->gpu.drawBatches,
      });
    }

    // Сбрасываем command buffer Vulkan текущего кадра.
    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex, renderDatas);

    // Semaphore Vulkan, которого ждет отправка команд.
    const VkSemaphore waitSemaphores[]          = {imageAvailableSemaphores_[currentFrame_]};
    // Стадия pipeline Vulkan, на которой ждем изображение.
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    // Semaphore Vulkan, который сигнализируется после рендера.
    const VkSemaphore signalSemaphores[]        = {renderFinishedSemaphores_[currentFrame_]};

    // Параметры отправки команд Vulkan в очередь.
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;   // Тип структуры отправки команд в очередь.
    submitInfo.waitSemaphoreCount   = 1;                               // Количество semaphores, которых ждет очередь.
    submitInfo.pWaitSemaphores      = waitSemaphores;                  // Semaphore доступности изображения swap-chain.
    submitInfo.pWaitDstStageMask    = waitStages;                      // Стадия pipeline, на которой выполняется ожидание.
    submitInfo.commandBufferCount   = 1;                               // Количество отправляемых command buffers.
    submitInfo.pCommandBuffers      = &commandBuffers_[currentFrame_]; // Command buffer текущего кадра.
    submitInfo.signalSemaphoreCount = 1;                               // Количество semaphores, которые будут просигналены.
    submitInfo.pSignalSemaphores    = signalSemaphores;                // Semaphore завершения рендеринга.

    // Отправляем command buffer Vulkan в графическую очередь.
    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
      throw std::runtime_error("aW9qE4sVrN :: failed to submit draw command buffer");
    }

    // Параметры показа изображения Vulkan.
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; // Тип структуры показа изображения.
    presentInfo.waitSemaphoreCount = 1;                                  // Количество semaphores перед показом.
    presentInfo.pWaitSemaphores    = signalSemaphores;                   // Ждем завершения рендеринга перед показом.
    presentInfo.swapchainCount     = 1;                                  // Количество swap-chains для показа.
    presentInfo.pSwapchains        = &swapChain_;                        // Swap-chain, из которого показываем изображение.
    presentInfo.pImageIndices      = &imageIndex;                        // Индекс изображения swap-chain для показа.

    // Показываем изображение swap-chain Vulkan.
    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("hT2pK8mJxC :: failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void VulkanInit::recreateSwapChain()
  {
    int width  = 0;
    int height = 0;
    // Получаем размер окна SDL в пикселях.
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    while (width == 0 || height == 0) {
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
    createSharedPipeline();
    createFramebuffers();
  }

  void VulkanInit::cleanupSwapChain()
  {
    for (const VkFramebuffer framebuffer : swapChainFramebuffers_) {
      // Уничтожаем framebuffer Vulkan.
      vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    swapChainFramebuffers_.clear();

    // Уничтожаем общий graphics pipeline Vulkan.
    if (sharedPipeline_ != VK_NULL_HANDLE) {
      vkDestroyPipeline(device_, sharedPipeline_, nullptr);
      sharedPipeline_ = VK_NULL_HANDLE;
    }
    if (sharedPipelineLayout_ != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(device_, sharedPipelineLayout_, nullptr);
      sharedPipelineLayout_ = VK_NULL_HANDLE;
    }

    // Уничтожаем render pass Vulkan.
    if (renderPass_ != VK_NULL_HANDLE) {
      vkDestroyRenderPass(device_, renderPass_, nullptr);
      renderPass_ = VK_NULL_HANDLE;
    }

    for (const VkImageView imageView : swapChainImageViews_) {
      // Уничтожаем image view Vulkan.
      vkDestroyImageView(device_, imageView, nullptr);
    }
    swapChainImageViews_.clear();

    // Уничтожаем swap-chain Vulkan.
    vkDestroySwapchainKHR(device_, swapChain_, nullptr);
    swapChain_ = VK_NULL_HANDLE;
  }

} // namespace app
