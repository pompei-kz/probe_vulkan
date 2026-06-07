module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <vector>

export module vulkanPipeline;

import util;
import cmd_pipeline;

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

  struct GeometryData
  {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
  };

} // namespace app

namespace app::vulkan_pipeline {

  export std::vector<Vertex> defaultVertices()
  {
    return {
        Vertex{{0.0F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.25F, 1.0F}},
        Vertex{{0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.25F, 1.0F}},
        Vertex{{-0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.25F, 1.0F}},
    };
  }

  export std::vector<uint32_t> defaultIndices()
  {
    return {0, 1, 2};
  }

  export void setSunLight(PushConstants &pushConstants, const glm::vec3 direction, const glm::vec3 color, const float force)
  {
    if (glm::dot(direction, direction) <= 0.0F) {
      throw std::invalid_argument("gP6vL1xZaE :: sun direction must be non-zero");
    }

    const glm::vec3 normalizedDirection = glm::normalize(direction);
    pushConstants.sunDirectionForce     = glm::vec4(normalizedDirection, force);
    pushConstants.sunColorAmbient       = glm::vec4(color, 0.18F);
  }

  export GeometryData buildShapeGroupGeometry(const std::vector<cmd::Mesh>     &meshes,
                                              const std::vector<cmd::Material> &materials,
                                              const std::vector<cmd::Shape>    &shapes)
  {
    GeometryData result;

    size_t vertexCount = 0;
    size_t indexCount  = 0;
    for (const cmd::Shape &shape : shapes) {
      if (shape.meshIndex >= meshes.size()) {
        throw std::out_of_range("sR4cN8vQpL :: shape meshIndex is out of range");
      }
      if (shape.materialIndex >= materials.size()) {
        throw std::out_of_range("dM7sK2rYqP :: shape materialIndex is out of range");
      }
      vertexCount += meshes[shape.meshIndex].points.size();
      indexCount += meshes[shape.meshIndex].triangles.size() * 3U;
    }

    result.vertices.reserve(vertexCount);
    result.indices.reserve(indexCount);

    for (const cmd::Shape &shape : shapes) {
      const cmd::Mesh &mesh          = meshes[shape.meshIndex];
      const glm::vec3 materialColor = materials[shape.materialIndex].color;

      const float angle = glm::length(shape.rotationVector);
      const glm::quat rotation =
          angle <= 0.000001F ? glm::quat(1.0F, 0.0F, 0.0F, 0.0F) : glm::angleAxis(angle, shape.rotationVector / angle);
      const glm::mat4 model =
          glm::translate(glm::mat4(1.0F), shape.position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0F), shape.scale);

      const uint32_t baseVertex = static_cast<uint32_t>(result.vertices.size());
      std::vector<glm::vec3> transformedPoints;
      std::vector<glm::vec3> normals(mesh.points.size(), glm::vec3(0.0F, 0.0F, 0.0F));
      transformedPoints.reserve(mesh.points.size());

      for (const glm::vec3 &point : mesh.points) {
        transformedPoints.push_back(glm::vec3(model * glm::vec4(point, 1.0F)));
      }

      for (const cmd::TriangleIdx &triangle : mesh.triangles) {
        if (triangle.index0 >= mesh.points.size() || triangle.index1 >= mesh.points.size() || triangle.index2 >= mesh.points.size()) {
          throw std::out_of_range("uX9mD2bKhT :: triangle vertex index is out of range");
        }
        const glm::vec3 edge0      = transformedPoints[triangle.index1] - transformedPoints[triangle.index0];
        const glm::vec3 edge1      = transformedPoints[triangle.index2] - transformedPoints[triangle.index0];
        const glm::vec3 faceNormal = glm::cross(edge1, edge0);
        normals[triangle.index0] += faceNormal;
        normals[triangle.index1] += faceNormal;
        normals[triangle.index2] += faceNormal;

        result.indices.push_back(baseVertex + triangle.index0);
        result.indices.push_back(baseVertex + triangle.index1);
        result.indices.push_back(baseVertex + triangle.index2);
      }

      for (size_t pointIndex = 0; pointIndex < transformedPoints.size(); ++pointIndex) {
        const glm::vec3 normal =
            glm::dot(normals[pointIndex], normals[pointIndex]) <= 0.0F ? glm::vec3(0.0F, 0.0F, 1.0F) : glm::normalize(normals[pointIndex]);
        result.vertices.push_back(Vertex{transformedPoints[pointIndex], normal, materialColor});
      }
    }

    return result;
  }

  export void recordCommandBuffer(const VkCommandBuffer            commandBuffer,
                                  const uint32_t                   imageIndex,
                                  const VkRenderPass               renderPass,
                                  const std::vector<VkFramebuffer> &swapChainFramebuffers,
                                  const VkExtent2D                 swapChainExtent,
                                  const VkPipeline                 graphicsPipeline,
                                  const VkBuffer                   vertexBuffer,
                                  const VkBuffer                   indexBuffer,
                                  const std::vector<uint32_t>      &indices,
                                  const VkPipelineLayout           pipelineLayout,
                                  const PushConstants              &pushConstants)
  {
    // Параметры начала записи command buffer Vulkan.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; // Тип структуры начала записи command buffer.

    // Начинаем запись command buffer Vulkan.
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("qN5eZ8rHsB :: failed to begin recording command buffer");
    }

    // Параметры начала render pass Vulkan.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; // Тип структуры начала render pass.
    renderPassInfo.renderPass        = renderPass;                               // Render pass, который нужно начать.
    renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];        // Framebuffer для текущего изображения swap-chain.
    renderPassInfo.renderArea.offset = {0, 0};                                   // Начало области рендеринга.
    renderPassInfo.renderArea.extent = swapChainExtent;                          // Размер области рендеринга.

    // Цвет очистки Vulkan.
    constexpr VkClearValue clearColor = {{{0.02F, 0.03F, 0.05F, 1.0F}}};
    renderPassInfo.clearValueCount    = 1;           // Количество значений очистки attachments.
    renderPassInfo.pClearValues       = &clearColor; // Цвет, которым очищается color attachment.

    // Начинаем render pass Vulkan.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // Привязываем графический pipeline Vulkan.
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    if (vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE || indices.empty()) {
      vkCmdEndRenderPass(commandBuffer);
      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("wJ4tK6mPxV :: failed to record command buffer");
      }
      return;
    }

    // Vertex buffer Vulkan для привязки к pipeline.
    const VkBuffer vertexBuffers[]   = {vertexBuffer};
    // Смещения vertex buffer Vulkan.
    constexpr VkDeviceSize offsets[] = {0};
    // Привязываем vertex buffer Vulkan.
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    // Привязываем index buffer Vulkan.
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    // Передаем матрицы трансформации в push constants Vulkan.
    vkCmdPushConstants(commandBuffer,
                       pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushConstants),
                       &pushConstants);
    // Отправляем индексированную команду рисования Vulkan.
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    // Завершаем render pass Vulkan.
    vkCmdEndRenderPass(commandBuffer);

    // Завершаем запись command buffer Vulkan.
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("wJ4tK6mPxV :: failed to record command buffer");
    }
  }

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
    const std::filesystem::path shaderPath     = util::executableBasePath() / "shaders";
    const std::vector<uint32_t> vertShaderCode = util::compileShader(shaderPath / "triangle.vert", shaderc_vertex_shader);
    const std::vector<uint32_t> fragShaderCode = util::compileShader(shaderPath / "triangle.frag", shaderc_fragment_shader);

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
