// ReSharper disable CppUseStructuredBinding
module;

#include <SDL3/SDL.h>

#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

module triangle_application;

import utils;
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
  };

  struct PipelineVk_ShapeGroup : PipelineVk
  {
    // TODO store here all what you need for draw this ShapeGroup pipeline
  };

  // TODO let it be pure abstract class
  struct LightVk
  {
    virtual ~LightVk() = default;
  };

  struct LightVk_Sun : LightVk
  {
    // TODO store here all what you need for use light of Sun in pipelines
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
      if (const auto command = std::dynamic_pointer_cast<cmd::CmdSequence>(cmdPtr)) {
        execute_CmdSequence(command);
        return;
      }

      std::cout << nowStr() << " t17HETHHeE :: Unknown cmd " << typeid(*pointer).name() << std::endl;
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    static void execute_CmdPrintToConsole(const std::shared_ptr<cmd::CmdPrintToConsole> cmdPtr)
    {
      std::cout << nowStr() << " " << cmdPtr->message << std::endl;
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    void execute_CmdSequence(const std::shared_ptr<cmd::CmdSequence> cmdPtr)
    {
      if (cmdPtr->sync) {
        for (cmd::CmdPtr subCmdPtr : cmdPtr->sequence) {
          execute_Cmd(subCmdPtr);
        }
        return;
      }

      {
        for (cmd::CmdPtr subCmdPtr : cmdPtr->sequence) {
          commands_.push_back(subCmdPtr);
        }
      }
    }

    void execute_CmdPipeline_ShapeGroup(const std::shared_ptr<cmd::CmdSetPipeline_ShapeGroup> cmdPtr)
    {
      // TODO Implement here command to add pipeline to draw ShapeGroup.
      // TODO pipeline stored in map: `this->pipeline_map_`
      // TODO pipelines must be draw in sequence of `this->pipeline_ids_`

      // TODO here you need initialize pipeline and put all parameters for pipeline in struct `PipelineVk_ShapeGroup` and put it to `this->pipeline_ids_`
    }

    void execute_CmdSetLight_Sun(const std::shared_ptr<cmd::CmdSetLight_Sun> cmdPtr)
    {
      // TODO Implement here command to add light of Sun.
      // TODO light stored in map: `this->light_map_`
      // TODO lights must be draw in sequence of `this->light_ids_`
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
