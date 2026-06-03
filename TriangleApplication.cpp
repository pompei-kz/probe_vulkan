// ReSharper disable CppUseStructuredBinding
module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

module triangle_application;

namespace
{

  constexpr int WINDOW_WIDTH         = 800;
  constexpr int WINDOW_HEIGHT        = 600;
  constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  // ReSharper disable once CppTemplateArgumentsCanBeDeduced
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
    VkSurfaceCapabilitiesKHR        capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
  };

  std::vector<char> readFile(const std::string &path)
  {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
      throw std::runtime_error("kQw7nPz4Lm :: failed to open " + path);
    }

    const auto        fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
  }

  std::string executableBasePath()
  {
    const char *basePath = SDL_GetBasePath();
    if (basePath == nullptr)
    {
      return "";
    }

    return basePath;
  }

} // namespace

struct TriangleApplication::Impl
{
  SDL_Window *window_ = nullptr;

  VkInstance       instance_       = VK_NULL_HANDLE;
  VkSurfaceKHR     surface_        = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice         device_         = VK_NULL_HANDLE;

  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  VkQueue presentQueue_  = VK_NULL_HANDLE;

  VkSwapchainKHR             swapChain_ = VK_NULL_HANDLE;
  std::vector<VkImage>       swapChainImages_;
  VkFormat                   swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
  VkExtent2D                 swapChainExtent_{};
  std::vector<VkImageView>   swapChainImageViews_;
  std::vector<VkFramebuffer> swapChainFramebuffers_;

  VkRenderPass     renderPass_       = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_   = VK_NULL_HANDLE;
  VkPipeline       graphicsPipeline_ = VK_NULL_HANDLE;

  VkCommandPool                commandPool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers_;

  std::vector<VkSemaphore> imageAvailableSemaphores_;
  std::vector<VkSemaphore> renderFinishedSemaphores_;
  std::vector<VkFence>     inFlightFences_;
  uint32_t                 currentFrame_       = 0;
  bool                     framebufferResized_ = false;

  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

  void initWindow()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      throw std::runtime_error(std::string("pR8mX2vNaQ :: ") + SDL_GetError());
    }

    window_ = SDL_CreateWindow("Vulkan Triangle", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr)
    {
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

    vkDeviceWaitIdle(device_);
  }

  void cleanup()
  {
    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
      vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
      vkDestroyFence(device_, inFlightFences_[i], nullptr);
    }

    vkDestroyCommandPool(device_, commandPool_, nullptr);
    vkDestroyDevice(device_, nullptr);
    SDL_Vulkan_DestroySurface(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  void createInstance()
  {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    Uint32             extensionCount = 0;
    const char *const *extensions     = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (extensions == nullptr)
    {
      throw std::runtime_error(std::string("hM4cV7nLxS :: ") + SDL_GetError());
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
    {
      throw std::runtime_error("uF2dK8wYpC :: failed to create Vulkan instance");
    }
  }

  void createSurface()
  {
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_))
    {
      throw std::runtime_error(std::string("zN6rQ1tGmH :: ERROR IN `SDL_Vulkan_CreateSurface()`: ") + SDL_GetError());
    }
  }

  void pickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
      throw std::runtime_error("bL3xS9eRwD :: failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
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
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
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
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
      if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
      {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = VK_FALSE;
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

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // ReSharper disable once CppTemplateArgumentsCanBeDeduced
    std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily};

    constexpr float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS)
    {
      throw std::runtime_error("cY7pD4nVaR :: failed to create logical device");
    }

    vkGetDeviceQueue(device_, *indices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, *indices.presentFamily, 0, &presentQueue_);
  }

  SwapChainSupport querySwapChainSupport(const VkPhysicalDevice device) const
  {
    SwapChainSupport details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if (formatCount != 0)
    {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
      details.presentModes.resize(presentModeCount);
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
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height))
    {
      throw std::runtime_error(std::string("mE9tH2wKsL :: ") + SDL_GetError());
    }

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width      = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height     = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }

  void createSwapChain()
  {
    const SwapChainSupport   swapChainSupport = querySwapChainSupport(physicalDevice_);
    const VkSurfaceFormatKHR surfaceFormat    = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR   presentMode      = chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D         extent           = chooseSwapExtent(swapChainSupport.capabilities);
    uint32_t                 imageCount       = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

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

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS)
    {
      throw std::runtime_error("xV5qN8cTpJ :: failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_      = extent;
  }

  void createImageViews()
  {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t i = 0; i < swapChainImages_.size(); ++i)
    {
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

      if (vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("rB1mF6zQeW :: failed to create image views");
      }
    }
  }

  void createRenderPass()
  {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat_;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS)
    {
      throw std::runtime_error("nK8sP3yLdM :: failed to create render pass");
    }
  }

  [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char> &code) const
  {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("gW4vC9hTxN :: failed to create shader module");
    }
    return shaderModule;
  }

  void createGraphicsPipeline()
  {
    const std::string basePath       = executableBasePath();
    const auto        vertShaderCode = readFile(basePath + "shaders/triangle.vert.spv");
    const auto        fragShaderCode = readFile(basePath + "shaders/triangle.frag.spv");

    const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x        = 0.0F;
    viewport.y        = 0.0F;
    viewport.width    = static_cast<float>(swapChainExtent_.width);
    viewport.height   = static_cast<float>(swapChainExtent_.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0F;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
    {
      throw std::runtime_error("dP6kR2mZaS :: failed to create pipeline layout");
    }

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

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS)
    {
      throw std::runtime_error("yL9fV5qBnE :: failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
  }

  void createFramebuffers()
  {
    swapChainFramebuffers_.resize(swapChainImageViews_.size());

    for (size_t i = 0; i < swapChainImageViews_.size(); ++i)
    {
      const VkImageView attachments[] = {swapChainImageViews_[i]};

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = renderPass_;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments    = attachments;
      framebufferInfo.width           = swapChainExtent_.width;
      framebufferInfo.height          = swapChainExtent_.height;
      framebufferInfo.layers          = 1;

      if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("tH3wX8cJpQ :: failed to create framebuffer");
      }
    }
  }

  void createCommandPool()
  {
    const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice_);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = *queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
    {
      throw std::runtime_error("sM7nD1vKgR :: failed to create command pool");
    }
  }

  void createCommandBuffers()
  {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool_;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
    {
      throw std::runtime_error("vC2pL9yWtF :: failed to allocate command buffers");
    }
  }

  void recordCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex) const
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("qN5eZ8rHsB :: failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass_;
    renderPassInfo.framebuffer       = swapChainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;

    constexpr VkClearValue clearColor = {{{0.02F, 0.03F, 0.05F, 1.0F}}};
    renderPassInfo.clearValueCount    = 1;
    renderPassInfo.pClearValues       = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

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

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("eR8sB2qNyT :: failed to create image-available semaphore");
      }

      if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("lD5vH9cWmK :: failed to create render-finished semaphore");
      }

      if (vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("pX1kT7zQaF :: failed to create in-flight fence");
      }
    }
  }

  void drawFrame()
  {
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
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

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    const VkSemaphore              waitSemaphores[]   = {imageAvailableSemaphores_[currentFrame_]};
    constexpr VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSemaphore              signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS)
    {
      throw std::runtime_error("aW9qE4sVrN :: failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapChain_;
    presentInfo.pImageIndices      = &imageIndex;

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
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    while (width == 0 || height == 0)
    {
      SDL_WaitEvent(nullptr);
      SDL_GetWindowSizeInPixels(window_, &width, &height);
    }

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
      vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    swapChainFramebuffers_.clear();

    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    graphicsPipeline_ = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    pipelineLayout_ = VK_NULL_HANDLE;

    vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;

    for (const VkImageView imageView : swapChainImageViews_)
    {
      vkDestroyImageView(device_, imageView, nullptr);
    }
    swapChainImageViews_.clear();

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
