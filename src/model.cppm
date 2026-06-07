module;

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

export module model;

import cmd_pipeline;

export namespace model {

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

  struct PipelineDrawData
  {
    VkPipeline                  graphicsPipeline = VK_NULL_HANDLE;
    VkBuffer                    vertexBuffer     = VK_NULL_HANDLE;
    VkBuffer                    indexBuffer      = VK_NULL_HANDLE;
    const std::vector<uint32_t> *indices         = nullptr;
    VkPipelineLayout            pipelineLayout   = VK_NULL_HANDLE;
  };

  struct VulkanPipelineDescriptors
  {
    // Layout графического pipeline Vulkan.
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    // Графический pipeline Vulkan.
    VkPipeline graphicsPipeline     = VK_NULL_HANDLE;
    // Vertex buffer Vulkan.
    VkBuffer vertexBuffer           = VK_NULL_HANDLE;
    // Память Vulkan для vertex buffer.
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    // Index buffer Vulkan.
    VkBuffer indexBuffer              = VK_NULL_HANDLE;
    // Память Vulkan для index buffer.
    VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
    std::vector<Vertex>    vertices;
    std::vector<uint32_t>  indices;
  };

  struct PipelineVk
  {
    virtual ~PipelineVk() = default;

    VulkanPipelineDescriptors descriptors;
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
