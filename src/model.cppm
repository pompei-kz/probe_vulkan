module;

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

export module model;

import cmd_pipeline;
import vulkanPipeline;

export namespace model {

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
    std::vector<app::Vertex> vertices;
    std::vector<uint32_t>    indices;
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
