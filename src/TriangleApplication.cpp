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
import pipeline;
import sync_map;
import sync_linked_list;
import triangle_application_vulkan_init;

namespace {

  constexpr int WINDOW_WIDTH  = 800;
  constexpr int WINDOW_HEIGHT = 600;

} // namespace

namespace app {

  struct Pipeline
  {
  };

  struct TriangleApplication::Impl
  {
    SDL_Window *window_            = nullptr;
    bool        framebufferResized_ = false;

    getter::Getter<Settings> &setting_;

    std::unordered_map<std::string, Pipeline> pipelines_;

    sync::SyncQueue<cmd::CmdPtr> commands;

    VulkanInit vulkan_;

    Impl(getter::Getter<Settings> &setting)
        : setting_(setting)
    {}

    void run(cmd::CmdFactory startCmdFactory)
    {
      initWindow();
      initVulkan();
      execute_Cmd(startCmdFactory());
      mainLoop();
      cleanup();
    }

    void execute_Cmd(cmd::CmdPtr cmdPtr)
    {
      const cmd::Cmd *pointer = cmdPtr.get();

      if (!pointer) return;

      if (const auto printToConsole = std::dynamic_pointer_cast<cmd::CmdPrintToConsole>(cmdPtr)) {
        execute_CmdPrintToConsole(printToConsole);
        return;
      }
      if (const auto printToConsole = std::dynamic_pointer_cast<cmd::CmdPipeline_ShapeGroup_Materials>(cmdPtr)) {
        execute_CmdPipeline_ShapeGroup_Materials(printToConsole);
        return;
      }

      std::cout << nowStr() << " t17HETHHeE :: Unknown cmd " << typeid(*pointer).name() << std::endl;
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    static void execute_CmdPrintToConsole(const std::shared_ptr<cmd::CmdPrintToConsole> cmdPtr)
    {
      std::cout << nowStr() << " " << cmdPtr->message << std::endl;
    }

    static void execute_CmdPipeline_ShapeGroup_Materials(const std::shared_ptr<cmd::CmdPipeline_ShapeGroup_Materials> cmdPtr)
    {
      // TODO implements it. Command applies pipeline: cmd::CmdPipeline_ShapeGroup_Materials.
      // TODO pipeline has id. If pipeline with this id already exists
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
  };

  TriangleApplication::TriangleApplication(getter::Getter<Settings> &setting)
      : impl_(std::make_unique<Impl>(setting))
  {}

  TriangleApplication::~TriangleApplication() = default;

  // ReSharper disable once CppMemberFunctionMayBeConst
  void TriangleApplication::run(cmd::CmdFactory startCmdGetter)
  {
    impl_->run(startCmdGetter);
  }

} // namespace app
