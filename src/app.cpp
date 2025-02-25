#include "app.hpp"
#include "SDL3/SDL_video.h"
#include "math/vec.hpp"
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL

#include <SDL3/SDL.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>

#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/render_time_widget.hpp"
#include "graphics/widgets/terrain_widget.hpp"
#include "graphics/widgets/util_widget.hpp"
#include "graphics/widgets/widget.hpp"
#include "util/error.hpp"
#include "world/chunk.hpp"
#include "world/player/camera.hpp"
#include "world/player/player.hpp"

namespace craft {
static SDL_GLContext ctx = nullptr;

App::~App() {
  if (ctx) {
    SDL_GL_DestroyContext(ctx);
  }

  SDL_Quit();
}

void App::ParseParameters(int argc, char **argv) {
  for (int i = 0; i < argc; ++i) {
  }
}

App::App(int argc, char **argv) {
  ParseParameters(argc, argv);

  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::Throw("SDL Initialization Failure", EF_AppendSDLErrors);
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::Throw("Vulkan Initialization Failure. Try updating your drivers. This application requires Vulkan to "
                        "operate, and must quit now.");
  }

  m_world.LoadChunks(0, 0, 0);

  m_window = std::make_shared<Window>(1024, 768, "test");
  m_player.RegisterWindowCallbacks(m_window);

  m_renderer = std::make_shared<vk::Renderer>(m_window, *m_player.GetCamera(), &m_world);

  m_widget_manager = std::make_shared<WidgetManager>();
  m_widget_manager->AddWidget(std::make_unique<UtilWidget>());
  m_widget_manager->AddWidget(std::make_unique<RenderTimingsWidget>(&time_taken_to_render));
  // m_widget_manager->AddWidget(std::make_unique<TerrainWidget>(m_regenerate, m_noise, m_regenerate_with_one_block,
  //                                                             m_scale_factor, m_max_height, m_current_block_type,
  //                                                             m_replace));
}

bool App::Run() {
  uint64_t start_tick = SDL_GetTicksNS();
  uint64_t end_tick = SDL_GetTicksNS();
  uint64_t stop = SDL_GetTicksNS();

  bool camera_enabled = true;

  uint64_t frame_60_average_render_time;
  int frames = 0;

  while (m_window->IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    start_tick = SDL_GetTicksNS();
    float tick_difference = start_tick - end_tick;
    end_tick = start_tick;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    m_widget_manager->RenderWidgets();

    uint64_t start = SDL_GetTicksNS();

    ImGui::Render();
    m_renderer->Draw();

    uint64_t end = SDL_GetTicksNS();
    uint64_t time_taken = end - start;

    m_window->PollEvents();

    // FIXME: temporary fix to make the tab button work properly
    if (m_window->IsKeyPressed(KeyboardKey::Tab) && (SDL_GetTicksNS() - stop) > 1e9) {
      m_window->ToggleRelativeMouseMode();
      camera_enabled = !camera_enabled;
      stop = SDL_GetTicksNS();
    }

    if (camera_enabled) {
      m_player.ProcessFrame(tick_difference / 1e9);

      if (m_window->IsKeyPressed(KeyboardKey::LCtrl)) {
        m_player.IncreaseMovementSpeed(5.0f);
      }

      if (m_window->IsButtonPressed(1)) {
      }

      auto [x, y] = m_window->GetRelativeMouseMotion();
      m_player.ProcessMouseMovement(x, y);
    }
  }

  return true;
}
} // namespace craft
