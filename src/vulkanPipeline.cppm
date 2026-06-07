module;

#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

export module vulkanPipeline;

import utils;

namespace {

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

} // namespace

export namespace app {

  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static VkVertexInputBindingDescription bindingDescription()
    {
      // Описание привязки vertex buffer Vulkan.
      VkVertexInputBindingDescription binding{};
      binding.binding   = 0;                           // Номер binding для vertex buffer.
      binding.stride    = sizeof(Vertex);              // Размер одной вершины в байтах.
      binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Данные читаются отдельно для каждой вершины.
      return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions()
    {
      // Описание атрибута позиции вершины Vulkan.
      std::array<VkVertexInputAttributeDescription, 3> attributes{};
      attributes[0].binding  = 0;                          // Binding, из которого читается атрибут.
      attributes[0].location = 0;                          // Location атрибута во входе vertex shader.
      attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT; // Формат позиции: три float компонента.
      attributes[0].offset   = offsetof(Vertex, position); // Смещение поля position внутри структуры Vertex.
      attributes[1].binding  = 0;
      attributes[1].location = 1;
      attributes[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributes[1].offset   = offsetof(Vertex, normal);
      attributes[2].binding  = 0;
      attributes[2].location = 2;
      attributes[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributes[2].offset   = offsetof(Vertex, color);
      return attributes;
    }
  };

  // Данные push constants Vulkan, которые передаются напрямую в vertex shader.
  // Структура должна совпадать с layout(push_constant) блоком в files/shaders/triangle.vert.
  struct PushConstants
  {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 sunDirectionForce;
    glm::vec4 sunColorAmbient;
  };

} // namespace app

namespace app::vulkan_pipeline {

  [[nodiscard]] VkShaderModule createShaderModule(const VkDevice device, const std::vector<uint32_t> &code)
  {
    // Параметры создания shader module Vulkan.
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; // Тип структуры создания shader module.
    createInfo.codeSize = code.size() * sizeof(uint32_t);              // Размер SPIR-V байткода в байтах.
    createInfo.pCode    = code.data();                                 // Указатель на SPIR-V байткод shader.

    // Дескриптор shader module Vulkan.
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    // Создаем shader module Vulkan.
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("gW4vC9hTxN :: failed to create shader module");
    }
    return shaderModule;
  }

  export void createGraphicsPipeline(const VkDevice   device,
                                     const VkExtent2D swapChainExtent,
                                     const VkRenderPass renderPass,
                                     VkPipelineLayout &pipelineLayout,
                                     VkPipeline       &graphicsPipeline)
  {
    const std::filesystem::path shaderPath     = executableBasePath() / "shaders";
    const std::vector<uint32_t> vertShaderCode = compileShader(shaderPath / "triangle.vert", shaderc_vertex_shader);
    const std::vector<uint32_t> fragShaderCode = compileShader(shaderPath / "triangle.frag", shaderc_fragment_shader);

    // Vertex shader module Vulkan.
    const VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    // Fragment shader module Vulkan.
    const VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    // Стадия vertex shader Vulkan.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // Тип структуры стадии shader pipeline.
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;                          // Стадия vertex shader.
    vertShaderStageInfo.module = vertShaderModule;                                    // Shader module с vertex shader.
    vertShaderStageInfo.pName  = "main";                                              // Точка входа в shader module.

    // Стадия fragment shader Vulkan.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; // Тип структуры стадии shader pipeline.
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;                        // Стадия fragment shader.
    fragShaderStageInfo.module = fragShaderModule;                                    // Shader module с fragment shader.
    fragShaderStageInfo.pName  = "main";                                              // Точка входа в shader module.

    // Список shader stages Vulkan.
    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Описание binding для vertex buffer Vulkan.
    const VkVertexInputBindingDescription bindingDescription = Vertex::bindingDescription();
    // Описание attributes для vertex buffer Vulkan.
    const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::attributeDescriptions();

    // Описание входных вершин Vulkan.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; // Тип структуры vertex input.
    vertexInputInfo.vertexBindingDescriptionCount   = 1;                                                         // Количество binding descriptions.
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;                                       // Описание шага и binding vertex buffer.
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());       // Количество vertex attributes.
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data(); // Описание формата и location vertex attributes.

    // Описание сборки примитивов Vulkan.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; // Тип структуры input assembly.
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Каждые три вершины образуют отдельный треугольник.
    inputAssembly.primitiveRestartEnable = VK_FALSE;                            // Primitive restart для индексов отключен.

    // Viewport Vulkan для области отрисовки.
    VkViewport viewport{};
    viewport.x        = 0.0F;                                        // Левая граница viewport.
    viewport.y        = 0.0F;                                        // Верхняя граница viewport.
    viewport.width    = static_cast<float>(swapChainExtent.width);  // Ширина viewport равна ширине swap-chain.
    viewport.height   = static_cast<float>(swapChainExtent.height); // Высота viewport равна высоте swap-chain.
    viewport.minDepth = 0.0F;                                        // Минимальная глубина viewport.
    viewport.maxDepth = 1.0F;                                        // Максимальная глубина viewport.

    // Scissor Vulkan для ограничения области отрисовки.
    VkRect2D scissor{};
    scissor.offset = {0, 0};           // Начало прямоугольника scissor.
    scissor.extent = swapChainExtent; // Размер scissor равен размеру swap-chain.

    // Состояние viewport/scissor Vulkan.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; // Тип структуры viewport state.
    viewportState.viewportCount = 1;                                                     // Количество viewport в pipeline.
    viewportState.pViewports    = &viewport;                                             // Описание viewport.
    viewportState.scissorCount  = 1;                                                     // Количество scissor rectangles.
    viewportState.pScissors     = &scissor;                                              // Описание scissor rectangle.

    // Состояние растеризации Vulkan.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; // Тип структуры rasterization state.
    rasterizer.depthClampEnable        = VK_FALSE;                                                   // Обрезаем фрагменты вне диапазона глубины.
    rasterizer.rasterizerDiscardEnable = VK_FALSE;                                                   // Растеризация включена.
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;                                       // Треугольники заполняются целиком.
    rasterizer.lineWidth               = 1.0F;                                                       // Толщина линий для line topology.
    rasterizer.cullMode                = VK_CULL_MODE_NONE;                                          // Отсечение граней отключено.
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE; // Вершины по часовой стрелке считаются лицевой стороной.
    rasterizer.depthBiasEnable         = VK_FALSE;                // Depth bias отключен.

    // Состояние multisampling Vulkan.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; // Тип структуры multisample state.
    multisampling.sampleShadingEnable  = VK_FALSE;                                                 // Sample shading отключен.
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Multisampling отключен, один sample на пиксель.

    // Настройки color blending Vulkan для attachment.
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Разрешаем запись RGBA каналов.
    colorBlendAttachment.blendEnable = VK_FALSE;                                                                   // Смешивание цветов отключено.

    // Состояние color blending Vulkan.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; // Тип структуры color blend state.
    colorBlending.logicOpEnable   = VK_FALSE;                                                 // Логические операции смешивания отключены.
    colorBlending.attachmentCount = 1;                                                        // Количество настроек blending для attachments.
    colorBlending.pAttachments    = &colorBlendAttachment;                                    // Настройки blending для color attachment.

    // Диапазон push constants Vulkan для матриц трансформации.
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Push constants доступны shader stages.
    pushConstantRange.offset     = 0;                          // Смещение диапазона push constants.
    pushConstantRange.size       = sizeof(PushConstants);      // Размер данных push constants.

    // Layout pipeline Vulkan.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; // Тип структуры создания pipeline layout.
    pipelineLayoutInfo.pushConstantRangeCount = 1;                                             // Количество диапазонов push constants.
    pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;                            // Описание диапазона push constants.

    // Создаем layout pipeline Vulkan.
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("dP6kR2mZaS :: failed to create pipeline layout");
    }

    // Параметры создания графического pipeline Vulkan.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // Тип структуры создания graphics pipeline.
    pipelineInfo.stageCount          = 2;                                               // Количество shader stages.
    pipelineInfo.pStages             = shaderStages;                                    // Описания vertex и fragment shader stages.
    pipelineInfo.pVertexInputState   = &vertexInputInfo;                                // Описание входных vertex данных.
    pipelineInfo.pInputAssemblyState = &inputAssembly;                                  // Описание сборки примитивов.
    pipelineInfo.pViewportState      = &viewportState;                                  // Описание viewport и scissor.
    pipelineInfo.pRasterizationState = &rasterizer;                                     // Описание растеризации.
    pipelineInfo.pMultisampleState   = &multisampling;                                  // Описание multisampling.
    pipelineInfo.pColorBlendState    = &colorBlending;                                  // Описание color blending.
    pipelineInfo.layout              = pipelineLayout;                                  // Pipeline layout с push constants.
    pipelineInfo.renderPass          = renderPass;                                      // Render pass, с которым совместим pipeline.
    pipelineInfo.subpass             = 0;                                               // Индекс subpass внутри render pass.

    // Создаем графический pipeline Vulkan.
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("yL9fV5qBnE :: failed to create graphics pipeline");
    }

    // Уничтожаем fragment shader module Vulkan.
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    // Уничтожаем vertex shader module Vulkan.
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
  }

} // namespace app::vulkan_pipeline
