// ReSharper disable CppUseStructuredBinding
module;

#include <SDL3/SDL.h>
#include <glm/vec3.hpp>

#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

module triangle_application;

import util;
import getter;
import settings;
import cmd;
import cmd_light;
import sync_map;
import sync_linked_list;
import triangle_application_vulkan_init;
import cmd_pipeline;

namespace app {

  constexpr int WINDOW_WIDTH  = 800;
  constexpr int WINDOW_HEIGHT = 600;

  // TODO let it be pure abstract class
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

  struct TriangleApplication::Impl
  {
    SDL_Window *window_             = nullptr;
    bool        framebufferResized_ = false;

    getter::Getter<Settings> &setting_;

    // TODO use this map to draw pipelines in Vulkan
    std::unordered_map<std::string, std::unique_ptr<PipelineVk>> pipeline_map_;
    std::vector<std::string>                                     pipeline_ids_; // TODO in this oder use pipelines

    // TODO use this map to use lights in Vulkan
    std::unordered_map<std::string, std::unique_ptr<LightVk>> light_map_;
    std::vector<std::string>                                  light_ids_; // TODO in this oder use lights

    sync::SyncQueue<cmd::CmdPtr> commands_;

    VulkanInit vulkan_;

    explicit Impl(getter::Getter<Settings> &setting)
        : setting_(setting)
    {}

    void run(const cmd::CmdFactory &&startCmdFactory)
    {
      initWindow();
      initVulkan();

      commands_.push_back(std::move(startCmdFactory()));

      mainLoop();
      cleanup();
    }

    void initWindow()
    {
      // Инициализируем видео-подсистему SDL.
      if (!SDL_Init(SDL_INIT_VIDEO)) {
        // Получаем текст ошибки SDL.
        throw std::runtime_error(std::string("pR8mX2vNaQ :: ") + SDL_GetError());
      }

      // Создаем окно SDL, совместимое с Vulkan.
      window_ = SDL_CreateWindow("Vulkan Triangle", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
      if (window_ == nullptr) {
        // Получаем текст ошибки SDL.
        throw std::runtime_error(std::string("aT5sJ9qBvE :: ") + SDL_GetError());
      }
    }

    void initVulkan()
    {
      vulkan_.setWindow(window_);
      vulkan_.createInstance();
      vulkan_.createSurface();
      vulkan_.pickPhysicalDevice();
      vulkan_.createLogicalDevice();
      vulkan_.createSwapChain();
      vulkan_.createImageViews();
      vulkan_.createRenderPass();
      vulkan_.createGraphicsPipeline();
      vulkan_.createFramebuffers();
      vulkan_.createCommandPool();
      vulkan_.createVertexBuffer();
      vulkan_.createIndexBuffer();
      vulkan_.createCommandBuffers();
      vulkan_.createSyncObjects();
    }

    void mainLoop()
    {
      bool quit = false;
      while (!quit) {
        SDL_Event event{};
        // Забираем следующее событие из очереди SDL.
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_EVENT_QUIT) {
            quit = true;
          } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            framebufferResized_ = true;
          }
        }

        executeAllCommands();
        updatePipelineRuntimeData();
        uploadPipelineRuntimeData();

        vulkan_.drawFrame(framebufferResized_);
      }

      vulkan_.waitIdle();
    }

    void cleanup()
    {
      vulkan_.cleanup();

      // Уничтожаем окно SDL.
      SDL_DestroyWindow(window_);

      // Завершаем работу SDL.
      SDL_Quit();
    }

    void executeAllCommands()
    {
      for (;;) {
        std::optional<cmd::CmdPtr> ref = commands_.pop_front();

        if (!ref.has_value()) return;

        execute_Cmd(ref.value());
      }
    }

    void updatePipelineRuntimeData()
    {
      for (const std::string &pipelineId : pipeline_ids_) {
        const auto pipelineIter = pipeline_map_.find(pipelineId);
        if (pipelineIter == pipeline_map_.end()) continue;

        auto *shapeGroup = dynamic_cast<PipelineVk_ShapeGroup *>(pipelineIter->second.get());
        if (shapeGroup == nullptr || !shapeGroup->shapeCountFn) continue;

        const size_t shapeCount = shapeGroup->shapeCountFn();
        if (shapeGroup->shapes.size() != shapeCount) {
          shapeGroup->shapes.resize(shapeCount);
        }
        if (shapeGroup->populateShapesFn) {
          shapeGroup->populateShapesFn(shapeGroup->shapes);
        }
      }
    }

    void uploadPipelineRuntimeData()
    {
      std::vector<VulkanPipelineDescriptors *> pipelineDescriptors;
      pipelineDescriptors.reserve(pipeline_ids_.size());

      for (const std::string &pipelineId : pipeline_ids_) {
        const auto pipelineIter = pipeline_map_.find(pipelineId);
        if (pipelineIter == pipeline_map_.end()) continue;

        auto *shapeGroup = dynamic_cast<PipelineVk_ShapeGroup *>(pipelineIter->second.get());
        if (shapeGroup == nullptr) continue;

        vulkan_.setShapeGroupData(shapeGroup->descriptors, shapeGroup->meshes, shapeGroup->materials, shapeGroup->shapes);
        pipelineDescriptors.push_back(&shapeGroup->descriptors);
      }

      vulkan_.setPipelineDescriptors(pipelineDescriptors);
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    void execute_Cmd(const cmd::CmdPtr cmdPtr)
    {
      const cmd::Cmd *pointer = cmdPtr.get();

      if (!pointer) return;

      if (const auto command = std::dynamic_pointer_cast<cmd::CmdPrintToConsole>(cmdPtr)) {
        execute_CmdPrintToConsole(command);
        return;
      }
      if (const auto command = std::dynamic_pointer_cast<cmd::CmdSetPipeline_ShapeGroup>(cmdPtr)) {
        execute_CmdPipeline_ShapeGroup(command);
        return;
      }
      if (const auto command = std::dynamic_pointer_cast<cmd::CmdSetLight_Sun>(cmdPtr)) {
        execute_CmdSetLight_Sun(command);
        return;
      }
      if (const auto command = std::dynamic_pointer_cast<cmd::CmdChangeCamera>(cmdPtr)) {
        execute_CmdChangeCamera(command);
        return;
      }
      if (const auto command = std::dynamic_pointer_cast<cmd::CmdSequence>(cmdPtr)) {
        execute_CmdSequence(command);
        return;
      }

      std::cout << util::nowStr() << " t17HETHHeE :: Unknown cmd " << typeid(*pointer).name() << std::endl;
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    static void execute_CmdPrintToConsole(const std::shared_ptr<cmd::CmdPrintToConsole> cmdPtr)
    {
      std::cout << util::nowStr() << " " << cmdPtr->message << std::endl;
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    void execute_CmdSequence(const std::shared_ptr<cmd::CmdSequence> cmdPtr)
    {
      for (const cmd::CmdPtr subCmdPtr : cmdPtr->sequence) {
        execute_Cmd(subCmdPtr);
      }
    }

    void execute_CmdPipeline_ShapeGroup(const std::shared_ptr<cmd::CmdSetPipeline_ShapeGroup> cmdPtr)
    {
      if (cmdPtr->id.empty()) {
        throw std::invalid_argument("yQ5nC8vLrB :: pipeline id must not be empty");
      }

      const auto existingPipelineIter = pipeline_map_.find(cmdPtr->id);
      const bool isNewPipeline        = existingPipelineIter == pipeline_map_.end();

      auto pipeline              = std::make_unique<PipelineVk_ShapeGroup>();
      pipeline->meshes           = cmdPtr->meshes;
      pipeline->materials        = cmdPtr->materials;
      pipeline->shapeCountFn     = cmdPtr->shapeCountFn;
      pipeline->populateShapesFn = cmdPtr->populateShapesFn;
      vulkan_.createPipelineDescriptors(pipeline->descriptors);

      if (!isNewPipeline) {
        vulkan_.destroyPipelineDescriptors(existingPipelineIter->second->descriptors);
      }

      pipeline_map_[cmdPtr->id] = std::move(pipeline);

      if (isNewPipeline) {
        pipeline_ids_.push_back(cmdPtr->id);
      }
    }

    void execute_CmdSetLight_Sun(const std::shared_ptr<cmd::CmdSetLight_Sun> cmdPtr)
    {
      if (cmdPtr->id.empty()) {
        throw std::invalid_argument("wB7tP2mXsK :: light id must not be empty");
      }

      const bool isNewLight = !light_map_.contains(cmdPtr->id);

      auto sun       = std::make_unique<LightVk_Sun>();
      sun->force     = cmdPtr->force;
      sun->direction = cmdPtr->direction;
      sun->color     = cmdPtr->color;

      vulkan_.setSunLight(sun->direction, sun->color, sun->force);

      std::unique_ptr<LightVk> light = std::move(sun);
      light_map_[cmdPtr->id]         = std::move(light);

      if (isNewLight) {
        light_ids_.push_back(cmdPtr->id);
      }
    }

    void execute_CmdChangeCamera(const std::shared_ptr<cmd::CmdChangeCamera> cmdPtr)
    {
      if (cmdPtr->positionApply) {
        vulkan_.setCameraPosition(cmdPtr->position);
      }
      if (cmdPtr->forwardApply) {
        vulkan_.setCameraForward(cmdPtr->forward);
      }
      if (cmdPtr->upApply) {
        vulkan_.setCameraUp(cmdPtr->up);
      }
      if (cmdPtr->planesApply) {
        vulkan_.setCameraPlanes(cmdPtr->planes.near, cmdPtr->planes.far);
      }
      if (cmdPtr->fovDegreesApply) {
        vulkan_.setCameraFovDegrees(cmdPtr->fovDegrees);
      }
    }
  };

  TriangleApplication::TriangleApplication(getter::Getter<Settings> &setting)
      : impl_(std::make_unique<Impl>(setting))
  {}

  TriangleApplication::~TriangleApplication() = default;

  // ReSharper disable once CppMemberFunctionMayBeConst
  void TriangleApplication::run(const cmd::CmdFactory &&startCmdGetter)
  {
    impl_->run(std::move(startCmdGetter));
  }

} // namespace app
