module;

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

export module model;

import cmd_pipeline;

export namespace model {

  // Количество кадров, которые CPU может подготавливать одновременно.
  constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  // Статическая вершина меша. Привязка 0, частота VK_VERTEX_INPUT_RATE_VERTEX.
  // Цвет больше не хранится в вершине - он берется из материала по materialIndex инстанса.
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;

    static VkVertexInputBindingDescription bindingDescription()
    {
      VkVertexInputBindingDescription binding{};
      binding.binding   = 0;
      binding.stride    = sizeof(Vertex);
      binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions()
    {
      std::array<VkVertexInputAttributeDescription, 2> attributes{};
      attributes[0].location = 0;
      attributes[0].binding  = 0;
      attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributes[0].offset   = offsetof(Vertex, position);
      attributes[1].location = 1;
      attributes[1].binding  = 0;
      attributes[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributes[1].offset   = offsetof(Vertex, normal);
      return attributes;
    }
  };

  // Данные одного инстанса. Привязка 1, частота VK_VERTEX_INPUT_RATE_INSTANCE.
  // Перезаписываются каждый кадр в кольцевой буфер инстансов.
  struct InstanceData
  {
    glm::mat4 model;
    uint32_t  materialIndex;
    uint32_t  pad0 = 0;
    uint32_t  pad1 = 0;
    uint32_t  pad2 = 0;

    static VkVertexInputBindingDescription bindingDescription()
    {
      VkVertexInputBindingDescription binding{};
      binding.binding   = 1;
      binding.stride    = sizeof(InstanceData);
      binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
      return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions()
    {
      std::array<VkVertexInputAttributeDescription, 5> attributes{};
      // Матрица модели передается как 4 отдельных vec4 в locations 2..5.
      for (uint32_t column = 0; column < 4; ++column) {
        attributes[column].location = 2 + column;
        attributes[column].binding  = 1;
        attributes[column].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[column].offset   = static_cast<uint32_t>(offsetof(InstanceData, model) + sizeof(glm::vec4) * column);
      }
      attributes[4].location = 6;
      attributes[4].binding  = 1;
      attributes[4].format   = VK_FORMAT_R32_UINT;
      attributes[4].offset   = static_cast<uint32_t>(offsetof(InstanceData, materialIndex));
      return attributes;
    }
  };

  // Материал в layout std430 для SSBO (set 1, binding 0).
  struct MaterialGpu
  {
    glm::vec4 color;
  };

  // Источник света в layout std430 для SSBO (set 0, binding 0).
  struct LightGpu
  {
    glm::vec4 directionType; // xyz - направление, w - тип (0 = направленный/солнце)
    glm::vec4 colorForce;    // rgb - цвет, w - сила
  };

  // Заголовок буфера света, предшествует массиву LightGpu в том же буфере (std430).
  struct LightBufferHeader
  {
    uint32_t  count;
    uint32_t  pad0 = 0;
    uint32_t  pad1 = 0;
    uint32_t  pad2 = 0;
    glm::vec4 ambient;
  };

  // Положение одного меша внутри общих статических vertex/index буферов pipeline.
  struct MeshRange
  {
    uint32_t firstIndex   = 0;
    uint32_t indexCount   = 0;
    int32_t  vertexOffset = 0;
  };

  // Одна инстансная команда рисования, пересчитывается каждый кадр.
  struct DrawBatch
  {
    MeshRange range;
    uint32_t  instanceCount = 0;
    uint32_t  firstInstance = 0;
  };

  // Данные push constants Vulkan. Должны совпадать с блоком layout(push_constant) в шейдере.
  struct PushConstants
  {
    glm::mat4 view;
    glm::mat4 projection;
  };

  // Результат построения статической геометрии меша.
  struct GeometryData
  {
    std::vector<Vertex>    vertices;
    std::vector<uint32_t>  indices;
    std::vector<MeshRange> meshRanges;
  };

  // Один слот кольцевого буфера, постоянно отображенный в память CPU.
  struct RingSlot
  {
    VkBuffer       buffer   = VK_NULL_HANDLE;
    VkDeviceMemory memory   = VK_NULL_HANDLE;
    void          *mapped   = nullptr;
    uint32_t       capacity = 0; // в элементах (инстансах или источниках света)
  };

  // Все Vulkan ресурсы, принадлежащие одному pipeline.
  struct PipelineGpu
  {
    // Статические ресурсы (создаются один раз при регистрации pipeline).
    VkBuffer               vertexBuffer   = VK_NULL_HANDLE;
    VkDeviceMemory         vertexMemory   = VK_NULL_HANDLE;
    VkBuffer               indexBuffer    = VK_NULL_HANDLE;
    VkDeviceMemory         indexMemory    = VK_NULL_HANDLE;
    VkBuffer               materialBuffer = VK_NULL_HANDLE;
    VkDeviceMemory         materialMemory = VK_NULL_HANDLE;
    std::vector<MeshRange> meshRanges;
    VkDescriptorPool       descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet        materialSet    = VK_NULL_HANDLE;

    // Динамические ресурсы (обновляются каждый кадр).
    std::array<RingSlot, MAX_FRAMES_IN_FLIGHT> instanceRing{};
    std::vector<DrawBatch>                     drawBatches;
  };

  struct PipelineVk
  {
    virtual ~PipelineVk() = default;

    PipelineGpu gpu;
  };

  struct PipelineVk_ShapeGroup : PipelineVk
  {
    std::vector<cmd::Mesh>                         meshes;
    std::vector<cmd::Material>                     materials;
    std::function<size_t()>                        shapeCountFn;
    std::function<void(std::vector<cmd::Shape> &)> populateShapesFn;
    std::vector<cmd::Shape>                        shapes;
  };

  struct LightVk
  {
    virtual ~LightVk() = default;
  };

  struct LightVk_Sun : LightVk
  {
    float     force = 1.0F;
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
    glm::vec3 color{1.0F, 1.0F, 1.0F};
  };

  // Данные, передаваемые в запись command buffer для одного pipeline.
  struct PipelineRenderData
  {
    VkDescriptorSet               materialSet    = VK_NULL_HANDLE;
    VkBuffer                      vertexBuffer   = VK_NULL_HANDLE;
    VkBuffer                      indexBuffer    = VK_NULL_HANDLE;
    VkBuffer                      instanceBuffer = VK_NULL_HANDLE;
    const std::vector<DrawBatch> *batches        = nullptr;
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool complete() const
    {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
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

  struct TransformMatrices
  {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
  };

} // namespace model
