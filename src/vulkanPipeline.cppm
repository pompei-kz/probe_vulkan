module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

export module vulkanPipeline;

import util;
import cmd_pipeline;
import model;

namespace app::vulkan_pipeline {

  export void setSunLight(model::PushConstants &pushConstants, const glm::vec3 direction, const glm::vec3 color, const float force)
  {
    if (glm::dot(direction, direction) <= 0.0F) {
      throw std::invalid_argument("gP6vL1xZaE :: sun direction must be non-zero");
    }

    const glm::vec3 normalizedDirection = glm::normalize(direction);
    pushConstants.sunDirectionForce     = glm::vec4(normalizedDirection, force);
    pushConstants.sunColorAmbient       = glm::vec4(color, 0.18F);
  }

  export model::GeometryData buildShapeGroupGeometry(const std::vector<cmd::Mesh>     &meshes,
                                                     const std::vector<cmd::Material> &materials,
                                                     const std::vector<cmd::Shape>    &shapes)
  {
    model::GeometryData result;

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
      const glm::vec3  materialColor = materials[shape.materialIndex].color;

      const float     angle    = glm::length(shape.rotationVector);
      const glm::quat rotation = angle <= 0.000001F ? glm::quat(1.0F, 0.0F, 0.0F, 0.0F) : glm::angleAxis(angle, shape.rotationVector / angle);
      const glm::mat4 model = glm::translate(glm::mat4(1.0F), shape.position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0F), shape.scale);

      const uint32_t         baseVertex = static_cast<uint32_t>(result.vertices.size());
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
        result.vertices.push_back(model::Vertex{transformedPoints[pointIndex], normal, materialColor});
      }
    }

    return result;
  }

  [[nodiscard]] uint32_t findMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
  {
    // Свойства памяти физического устройства Vulkan.
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    // Получаем свойства памяти физического устройства Vulkan.
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
      // ReSharper disable once CppRedundantParentheses
      if ((typeFilter & (1 << i)) != 0 && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }

    throw std::runtime_error("nH4pT7wXaC :: failed to find suitable Vulkan memory type");
  }

  void createBuffer(const VkDevice              device,
                    const VkPhysicalDevice      physicalDevice,
                    const VkDeviceSize          size,
                    const VkBufferUsageFlags    usage,
                    const VkMemoryPropertyFlags properties,
                    VkBuffer                   &buffer,
                    VkDeviceMemory             &bufferMemory)
  {
    // Параметры создания buffer Vulkan.
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; // Тип структуры создания buffer.
    bufferInfo.size        = size;                                 // Размер buffer в байтах.
    bufferInfo.usage       = usage;                                // Назначение buffer: vertex, index или другое.
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;            // Buffer используется одним семейством очередей.

    // Создаем buffer Vulkan.
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
      throw std::runtime_error("bQ8mS2vPrL :: failed to create Vulkan buffer");
    }

    // Требования памяти Vulkan для buffer.
    VkMemoryRequirements memoryRequirements{};
    // Получаем требования памяти Vulkan для buffer.
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    // Параметры выделения памяти Vulkan.
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;                                        // Тип структуры выделения памяти.
    allocInfo.allocationSize  = memoryRequirements.size;                                                       // Размер выделяемой памяти.
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties); // Индекс подходящего типа памяти.

    // Выделяем память Vulkan для buffer.
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("kV6xM9pNdE :: failed to allocate Vulkan buffer memory");
    }

    // Привязываем память Vulkan к buffer.
    if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS) {
      throw std::runtime_error("rC3tL8yHsW :: failed to bind Vulkan buffer memory");
    }
  }

  void uploadBufferData(const VkDevice device, const VkDeviceMemory bufferMemory, const void *source, const VkDeviceSize size)
  {
    void *data = nullptr;
    // Отображаем память Vulkan в адресное пространство CPU.
    if (vkMapMemory(device, bufferMemory, 0, size, 0, &data) != VK_SUCCESS) {
      throw std::runtime_error("uP2eN5qKtB :: failed to map Vulkan buffer memory");
    }
    std::memcpy(data, source, size);
    // Завершаем отображение памяти Vulkan.
    vkUnmapMemory(device, bufferMemory);
  }

  export void destroyPipelineGeometryBuffers(const VkDevice device, model::VulkanPipelineDescriptors &pipelineDescriptors)
  {
    if (device == VK_NULL_HANDLE) return;

    if (pipelineDescriptors.indexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, pipelineDescriptors.indexBuffer, nullptr);
      pipelineDescriptors.indexBuffer = VK_NULL_HANDLE;
    }
    if (pipelineDescriptors.indexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, pipelineDescriptors.indexBufferMemory, nullptr);
      pipelineDescriptors.indexBufferMemory = VK_NULL_HANDLE;
    }
    if (pipelineDescriptors.vertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, pipelineDescriptors.vertexBuffer, nullptr);
      pipelineDescriptors.vertexBuffer = VK_NULL_HANDLE;
    }
    if (pipelineDescriptors.vertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, pipelineDescriptors.vertexBufferMemory, nullptr);
      pipelineDescriptors.vertexBufferMemory = VK_NULL_HANDLE;
    }
  }

  export void createPipelineVertexBuffer(const VkDevice                    device,
                                         const VkPhysicalDevice            physicalDevice,
                                         model::VulkanPipelineDescriptors &pipelineDescriptors)
  {
    if (pipelineDescriptors.vertices.empty()) return;

    const VkDeviceSize bufferSize = sizeof(pipelineDescriptors.vertices[0]) * pipelineDescriptors.vertices.size();
    createBuffer(device,
                 physicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 pipelineDescriptors.vertexBuffer,
                 pipelineDescriptors.vertexBufferMemory);
    uploadBufferData(device, pipelineDescriptors.vertexBufferMemory, pipelineDescriptors.vertices.data(), bufferSize);
  }

  export void createPipelineIndexBuffer(const VkDevice                    device,
                                        const VkPhysicalDevice            physicalDevice,
                                        model::VulkanPipelineDescriptors &pipelineDescriptors)
  {
    if (pipelineDescriptors.indices.empty()) return;

    const VkDeviceSize bufferSize = sizeof(pipelineDescriptors.indices[0]) * pipelineDescriptors.indices.size();
    createBuffer(device,
                 physicalDevice,
                 bufferSize,
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 pipelineDescriptors.indexBuffer,
                 pipelineDescriptors.indexBufferMemory);
    uploadBufferData(device, pipelineDescriptors.indexBufferMemory, pipelineDescriptors.indices.data(), bufferSize);
  }

  export void recreatePipelineGeometryBuffers(const VkDevice                    device,
                                              const VkPhysicalDevice            physicalDevice,
                                              model::VulkanPipelineDescriptors &pipelineDescriptors)
  {
    if (device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device);
    destroyPipelineGeometryBuffers(device, pipelineDescriptors);
    createPipelineVertexBuffer(device, physicalDevice, pipelineDescriptors);
    createPipelineIndexBuffer(device, physicalDevice, pipelineDescriptors);
  }

  export void setShapeGroupData(const VkDevice                    device,
                                const VkPhysicalDevice            physicalDevice,
                                model::VulkanPipelineDescriptors &pipelineDescriptors,
                                const std::vector<cmd::Mesh>     &meshes,
                                const std::vector<cmd::Material> &materials,
                                const std::vector<cmd::Shape>    &shapes)
  {
    model::GeometryData geometry = buildShapeGroupGeometry(meshes, materials, shapes);
    pipelineDescriptors.vertices = std::move(geometry.vertices);
    pipelineDescriptors.indices  = std::move(geometry.indices);
    recreatePipelineGeometryBuffers(device, physicalDevice, pipelineDescriptors);
  }

  void recordPipelineDraw(const VkCommandBuffer commandBuffer, const model::PipelineDrawData &pipelineDraw, const model::PushConstants &pushConstants)
  {
    if (pipelineDraw.graphicsPipeline == VK_NULL_HANDLE || pipelineDraw.pipelineLayout == VK_NULL_HANDLE) {
      return;
    }

    // Привязываем графический pipeline Vulkan.
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineDraw.graphicsPipeline);

    if (pipelineDraw.vertexBuffer == VK_NULL_HANDLE || pipelineDraw.indexBuffer == VK_NULL_HANDLE || pipelineDraw.indices == nullptr ||
        pipelineDraw.indices->empty()) {
      return;
    }

    // Vertex buffer Vulkan для привязки к pipeline.
    const VkBuffer vertexBuffers[]   = {pipelineDraw.vertexBuffer};
    // Смещения vertex buffer Vulkan.
    constexpr VkDeviceSize offsets[] = {0};
    // Привязываем vertex buffer Vulkan.
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    // Привязываем index buffer Vulkan.
    vkCmdBindIndexBuffer(commandBuffer, pipelineDraw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    // Передаем матрицы трансформации в push constants Vulkan.
    vkCmdPushConstants(commandBuffer,
                       pipelineDraw.pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(model::PushConstants),
                       &pushConstants);
    // Отправляем индексированную команду рисования Vulkan.
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(pipelineDraw.indices->size()), 1, 0, 0, 0);
  }

  export void recordCommandBuffer(const VkCommandBuffer                       commandBuffer,
                                  const uint32_t                              imageIndex,
                                  const VkRenderPass                          renderPass,
                                  const std::vector<VkFramebuffer>           &swapChainFramebuffers,
                                  const VkExtent2D                            swapChainExtent,
                                  const std::vector<model::PipelineDrawData> &pipelineDraws,
                                  const model::PushConstants                 &pushConstants)
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

    for (const model::PipelineDrawData &pipelineDraw : pipelineDraws) {
      recordPipelineDraw(commandBuffer, pipelineDraw, pushConstants);
    }

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

  export void createGraphicsPipeline(const VkDevice     device,
                                     const VkExtent2D   swapChainExtent,
                                     const VkRenderPass renderPass,
                                     VkPipelineLayout  &pipelineLayout,
                                     VkPipeline        &graphicsPipeline)
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
    const VkVertexInputBindingDescription bindingDescription                     = model::Vertex::bindingDescription();
    // Описание attributes для vertex buffer Vulkan.
    const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = model::Vertex::attributeDescriptions();

    // Описание входных вершин Vulkan.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; // Тип структуры vertex input.
    vertexInputInfo.vertexBindingDescriptionCount   = 1;                                                         // Количество binding descriptions.
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;                                 // Описание шага и binding vertex buffer.
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // Количество vertex attributes.
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data(); // Описание формата и location vertex attributes.

    // Описание сборки примитивов Vulkan.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; // Тип структуры input assembly.
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Каждые три вершины образуют отдельный треугольник.
    inputAssembly.primitiveRestartEnable = VK_FALSE;                            // Primitive restart для индексов отключен.

    // Viewport Vulkan для области отрисовки.
    VkViewport viewport{};
    viewport.x        = 0.0F;                                       // Левая граница viewport.
    viewport.y        = 0.0F;                                       // Верхняя граница viewport.
    viewport.width    = static_cast<float>(swapChainExtent.width);  // Ширина viewport равна ширине swap-chain.
    viewport.height   = static_cast<float>(swapChainExtent.height); // Высота viewport равна высоте swap-chain.
    viewport.minDepth = 0.0F;                                       // Минимальная глубина viewport.
    viewport.maxDepth = 1.0F;                                       // Максимальная глубина viewport.

    // Scissor Vulkan для ограничения области отрисовки.
    VkRect2D scissor{};
    scissor.offset = {0, 0};          // Начало прямоугольника scissor.
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
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;                                    // Multisampling отключен, один sample на пиксель.

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
    pushConstantRange.offset     = 0;                                                         // Смещение диапазона push constants.
    pushConstantRange.size       = sizeof(model::PushConstants);                              // Размер данных push constants.

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
