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

  // Строит общие статические vertex/index буферы для всех мешей pipeline.
  // Нормали вычисляются один раз в локальной системе координат меша.
  // Индексы остаются локальными для меша (база добавляется через vertexOffset при рисовании).
  export model::GeometryData buildStaticMesh(const std::vector<cmd::Mesh> &meshes)
  {
    model::GeometryData out;
    out.meshRanges.resize(meshes.size());

    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
      const cmd::Mesh &mesh = meshes[meshIndex];

      model::MeshRange range;
      range.vertexOffset = static_cast<int32_t>(out.vertices.size());
      range.firstIndex   = static_cast<uint32_t>(out.indices.size());

      std::vector<glm::vec3> normals(mesh.points.size(), glm::vec3(0.0F, 0.0F, 0.0F));

      for (const cmd::TriangleIdx &triangle : mesh.triangles) {
        if (triangle.index0 >= mesh.points.size() || triangle.index1 >= mesh.points.size() || triangle.index2 >= mesh.points.size()) {
          throw std::out_of_range("uX9mD2bKhT :: triangle vertex index is out of range");
        }
        const glm::vec3 edge0      = mesh.points[triangle.index1] - mesh.points[triangle.index0];
        const glm::vec3 edge1      = mesh.points[triangle.index2] - mesh.points[triangle.index0];
        const glm::vec3 faceNormal = glm::cross(edge1, edge0);
        normals[triangle.index0] += faceNormal;
        normals[triangle.index1] += faceNormal;
        normals[triangle.index2] += faceNormal;

        out.indices.push_back(triangle.index0);
        out.indices.push_back(triangle.index1);
        out.indices.push_back(triangle.index2);
      }

      for (size_t pointIndex = 0; pointIndex < mesh.points.size(); ++pointIndex) {
        const glm::vec3 normal =
            glm::dot(normals[pointIndex], normals[pointIndex]) <= 0.0F ? glm::vec3(0.0F, 0.0F, 1.0F) : glm::normalize(normals[pointIndex]);
        out.vertices.push_back(model::Vertex{mesh.points[pointIndex], normal});
      }

      range.indexCount        = static_cast<uint32_t>(out.indices.size()) - range.firstIndex;
      out.meshRanges[meshIndex] = range;
    }

    return out;
  }

  // Строит матрицу модели одного shape: T * R * S.
  export glm::mat4 shapeModelMatrix(const cmd::Shape &shape)
  {
    const float     angle    = glm::length(shape.rotationVector);
    const glm::quat rotation = angle <= 0.000001F ? glm::quat(1.0F, 0.0F, 0.0F, 0.0F) : glm::angleAxis(angle, shape.rotationVector / angle);
    return glm::translate(glm::mat4(1.0F), shape.position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0F), shape.scale);
  }

  // Группирует shapes по meshIndex методом подсчета (counting sort) и записывает
  // данные инстансов в `dst` так, что инстансы одного меша лежат подряд.
  // Возвращает список инстансных команд рисования (по одной на меш).
  export std::vector<model::DrawBatch> groupShapes(const std::vector<cmd::Shape>        &shapes,
                                                   const std::vector<model::MeshRange>  &meshRanges,
                                                   model::InstanceData                  *dst,
                                                   const uint32_t                        capacity,
                                                   const uint32_t                        materialCount)
  {
    const size_t meshCount = meshRanges.size();

    std::vector<uint32_t> counts(meshCount, 0);
    for (const cmd::Shape &shape : shapes) {
      if (shape.meshIndex < meshCount) {
        ++counts[shape.meshIndex];
      }
    }

    std::vector<model::DrawBatch> batches;
    std::vector<uint32_t>         base(meshCount, 0);
    uint32_t                      running = 0;
    for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex) {
      base[meshIndex] = running;
      if (counts[meshIndex] > 0 && meshRanges[meshIndex].indexCount > 0) {
        batches.push_back(model::DrawBatch{meshRanges[meshIndex], counts[meshIndex], running});
      }
      running += counts[meshIndex];
    }

    if (dst == nullptr) {
      return batches;
    }

    std::vector<uint32_t> cursor = base;
    for (const cmd::Shape &shape : shapes) {
      if (shape.meshIndex >= meshCount) {
        continue;
      }
      const uint32_t slot = cursor[shape.meshIndex]++;
      if (slot >= capacity) {
        continue; // защита от переполнения буфера
      }
      const uint32_t materialIndex = materialCount == 0 ? 0 : (shape.materialIndex < materialCount ? shape.materialIndex : materialCount - 1);

      model::InstanceData instance{};
      instance.model         = shapeModelMatrix(shape);
      instance.materialIndex = materialIndex;
      dst[slot]              = instance;
    }

    return batches;
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

  export void createBuffer(const VkDevice              device,
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

  export void uploadBufferData(const VkDevice device, const VkDeviceMemory bufferMemory, const void *source, const VkDeviceSize size)
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

  // Создает descriptor set layout для света (set 0) и материалов (set 1).
  // Оба - один storage buffer, доступный во fragment shader.
  export void createDescriptorSetLayouts(const VkDevice device, VkDescriptorSetLayout &globalSetLayout, VkDescriptorSetLayout &materialSetLayout)
  {
    VkDescriptorSetLayoutBinding storageBinding{};
    storageBinding.binding         = 0;
    storageBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBinding.descriptorCount = 1;
    storageBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &storageBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &globalSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("hQ7sV2nLpD :: failed to create global descriptor set layout");
    }
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &materialSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("kF3mT8xRwB :: failed to create material descriptor set layout");
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

  export void createGraphicsPipeline(const VkDevice              device,
                                     const VkExtent2D            swapChainExtent,
                                     const VkRenderPass          renderPass,
                                     const VkDescriptorSetLayout globalSetLayout,
                                     const VkDescriptorSetLayout materialSetLayout,
                                     VkPipelineLayout           &pipelineLayout,
                                     VkPipeline                 &graphicsPipeline)
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

    // Две привязки: 0 - вершины меша (per-vertex), 1 - данные инстансов (per-instance).
    const VkVertexInputBindingDescription bindingDescriptions[] = {
        model::Vertex::bindingDescription(),
        model::InstanceData::bindingDescription(),
    };

    const std::array<VkVertexInputAttributeDescription, 2> vertexAttributes   = model::Vertex::attributeDescriptions();
    const std::array<VkVertexInputAttributeDescription, 5> instanceAttributes = model::InstanceData::attributeDescriptions();

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.reserve(vertexAttributes.size() + instanceAttributes.size());
    attributeDescriptions.insert(attributeDescriptions.end(), vertexAttributes.begin(), vertexAttributes.end());
    attributeDescriptions.insert(attributeDescriptions.end(), instanceAttributes.begin(), instanceAttributes.end());

    // Описание входных вершин Vulkan.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 2;
    vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // Описание сборки примитивов Vulkan.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport Vulkan для области отрисовки.
    VkViewport viewport{};
    viewport.x        = 0.0F;
    viewport.y        = 0.0F;
    viewport.width    = static_cast<float>(swapChainExtent.width);
    viewport.height   = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    // Scissor Vulkan для ограничения области отрисовки.
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

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
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // Состояние multisampling Vulkan.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Настройки color blending Vulkan для attachment.
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // Состояние color blending Vulkan.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // Диапазон push constants Vulkan для матриц камеры (view, projection).
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset     = 0;
    pushConstantRange.size       = sizeof(model::PushConstants);

    // Layout pipeline Vulkan с двумя descriptor sets (свет + материалы).
    const VkDescriptorSetLayout setLayouts[] = {globalSetLayout, materialSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 2;
    pipelineLayoutInfo.pSetLayouts            = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

    // Создаем layout pipeline Vulkan.
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      vkDestroyShaderModule(device, fragShaderModule, nullptr);
      vkDestroyShaderModule(device, vertShaderModule, nullptr);
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
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;

    // Создаем графический pipeline Vulkan.
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
      vkDestroyShaderModule(device, fragShaderModule, nullptr);
      vkDestroyShaderModule(device, vertShaderModule, nullptr);
      throw std::runtime_error("yL9fV5qBnE :: failed to create graphics pipeline");
    }

    // Уничтожаем shader modules Vulkan.
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
  }

  void recordPipelineDraw(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout, const model::PipelineRenderData &renderData)
  {
    if (renderData.vertexBuffer == VK_NULL_HANDLE || renderData.indexBuffer == VK_NULL_HANDLE || renderData.instanceBuffer == VK_NULL_HANDLE ||
        renderData.materialSet == VK_NULL_HANDLE || renderData.batches == nullptr || renderData.batches->empty()) {
      return;
    }

    // Привязываем descriptor set материалов этого pipeline (set 1).
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &renderData.materialSet, 0, nullptr);

    // Привязываем vertex buffer (binding 0) и instance buffer (binding 1).
    const VkBuffer     buffers[] = {renderData.vertexBuffer, renderData.instanceBuffer};
    const VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, renderData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // По одной инстансной команде рисования на меш.
    for (const model::DrawBatch &batch : *renderData.batches) {
      vkCmdDrawIndexed(commandBuffer,
                       batch.range.indexCount,
                       batch.instanceCount,
                       batch.range.firstIndex,
                       batch.range.vertexOffset,
                       batch.firstInstance);
    }
  }

  export void recordCommandBuffer(const VkCommandBuffer                          commandBuffer,
                                  const uint32_t                                 imageIndex,
                                  const VkRenderPass                             renderPass,
                                  const std::vector<VkFramebuffer>              &swapChainFramebuffers,
                                  const VkExtent2D                               swapChainExtent,
                                  const VkPipeline                               graphicsPipeline,
                                  const VkPipelineLayout                         pipelineLayout,
                                  const VkDescriptorSet                          lightSet,
                                  const model::PushConstants                    &pushConstants,
                                  const std::vector<model::PipelineRenderData>  &renderDatas)
  {
    // Параметры начала записи command buffer Vulkan.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Начинаем запись command buffer Vulkan.
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("qN5eZ8rHsB :: failed to begin recording command buffer");
    }

    // Параметры начала render pass Vulkan.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    // Цвет очистки Vulkan.
    constexpr VkClearValue clearColor = {{{0.02F, 0.03F, 0.05F, 1.0F}}};
    renderPassInfo.clearValueCount    = 1;
    renderPassInfo.pClearValues       = &clearColor;

    // Начинаем render pass Vulkan.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (graphicsPipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE) {
      // Общий pipeline и push constants привязываются один раз на кадр.
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
      vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(model::PushConstants), &pushConstants);

      // Descriptor set света (set 0) общий для всех pipeline, привязывается один раз.
      if (lightSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &lightSet, 0, nullptr);
      }

      // Рисуем каждый зарегистрированный pipeline.
      for (const model::PipelineRenderData &renderData : renderDatas) {
        recordPipelineDraw(commandBuffer, pipelineLayout, renderData);
      }
    }

    // Завершаем render pass Vulkan.
    vkCmdEndRenderPass(commandBuffer);

    // Завершаем запись command buffer Vulkan.
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("wJ4tK6mPxV :: failed to record command buffer");
    }
  }

} // namespace app::vulkan_pipeline
