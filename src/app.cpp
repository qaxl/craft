#include "app.hpp"

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>

#include "graphics/camera.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/render_time_widget.hpp"
#include "graphics/widgets/terrain_widget.hpp"
#include "graphics/widgets/util_widget.hpp"
#include "graphics/widgets/widget.hpp"
#include "util/error.hpp"
#include "world/chunk.hpp"
#include "world/generator.hpp"

namespace craft {
App::~App() {
  SDL_Quit();
  // SteamAPI_Shutdown();
}

App::App() {
  // if (!SteamAPI_Init()) {
  //   // TODO: not make this forced.
  //   RuntimeError::Throw("Steamworks API couldn't initialize.");
  // }

  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::Throw("SDL Initialization Failure", EF_AppendSDLErrors);
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::Throw(
        "Vulkan Initialization Failure: Couldn't load Vulkan library functions. Does this computer support rendering "
        "with Vulkan? Currently, we do not support graphics APIs other than Vulkan.\nOpenGL support may be added in "
        "the "
        "future for better compatibility with older hardware.\nIf you have hardware that is newer than, let's say, the "
        "last 5 to 10 years, then it most likely does support Vulkan 1.3. If that's the case, consider updating your "
        "graphics drivers.\n\nRead more for AMD graphics cards: "
        "https://www.amd.com/en/support/download/drivers.html\nRead more for NVIDIA graphics cards: "
        "https://www.nvidia.com/en-us/drivers/\n\nIf upgrading your drivers doesn't fix this issue, you most likely do "
        "not have support for Vulkan 1.3, which is the minimum required version for this application to run.\nAnd if "
        "you "
        "have the ability to program in OpenGL/DirectX 11, proficiently, you're welcome contributing into the "
        "framework that the current application is running from https://github.com/qaxl/craft :)");
  }
  m_chunk = std::make_unique<Chunk>();
  GenerateChunk(false, 10.0f, m_noise, *m_chunk);

  m_window = std::make_shared<Window>(1024, 768, "test");
  m_renderer = std::make_shared<vk::Renderer>(m_window, m_camera, *m_chunk);

  m_widget_manager = std::make_shared<WidgetManager>();
  m_widget_manager->AddWidget(std::make_unique<UtilWidget>());
  m_widget_manager->AddWidget(std::make_unique<RenderTimingsWidget>(&time_taken_to_render));
  m_widget_manager->AddWidget(std::make_unique<TerrainWidget>(m_regenerate, m_noise, m_regenerate_with_one_block,
                                                              m_scale_factor, m_max_height));
}

bool App::Run() {
  uint64_t start_tick = SDL_GetTicksNS();
  uint64_t end_tick = SDL_GetTicksNS();
  uint64_t stop = SDL_GetTicksNS();

  bool camera_enabled = true;

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
    time_taken_to_render = static_cast<float>(time_taken) / 1000.0f / 1000.0f;

    m_window->PollEvents();

    // FIXME: temporary fix to make the tab button work properly
    if (m_window->IsKeyPressed(KeyboardKey::Tab) && (SDL_GetTicksNS() - stop) > 1e9) {
      m_window->ToggleRelativeMouseMode();
      camera_enabled = !camera_enabled;
      stop = SDL_GetTicksNS();
    }

    if (camera_enabled) {
      if (m_window->IsKeyPressed(KeyboardKey::W)) {
        m_camera.ProcessKeyboard(CameraMovement::Forward, tick_difference * 1e-9);
      }
      if (m_window->IsKeyPressed(KeyboardKey::S)) {
        m_camera.ProcessKeyboard(CameraMovement::Backward, tick_difference * 1e-9);
      }
      if (m_window->IsKeyPressed(KeyboardKey::A)) {
        m_camera.ProcessKeyboard(CameraMovement::Left, tick_difference * 1e-9);
      }
      if (m_window->IsKeyPressed(KeyboardKey::D)) {
        m_camera.ProcessKeyboard(CameraMovement::Right, tick_difference * 1e-9);
      }

      if (m_window->IsKeyPressed(KeyboardKey::LShift)) {
        m_camera.ProcessKeyboard(CameraMovement::Down, tick_difference * 1e-9);
      }
      if (m_window->IsKeyPressed(KeyboardKey::Space)) {
        m_camera.ProcessKeyboard(CameraMovement::Up, tick_difference * 1e-9);
      }

      if (m_window->IsKeyPressed(KeyboardKey::KP_0)) {
        m_camera.IncreaseMovementSpeedBy(1.0f);
      }
      if (m_window->IsKeyPressed(KeyboardKey::KP_1)) {
        m_camera.IncreaseMovementSpeedBy(-1.0f);
      }

      if (m_window->IsButtonPressed(1)) {
        // TODO: implement raycast algorithm to break blocks
      }

      auto [x, y] = m_window->GetRelativeMouseMotion();
      m_camera.ProcessMouseMovement(x, y);
      auto scroll_y = m_window->GetMouseScroll();
      m_camera.ProcessMouseScroll(scroll_y);
    }

    if (m_regenerate) {
      GenerateChunk(m_regenerate_with_one_block, m_max_height, m_noise, *m_chunk, m_scale_factor);
      m_renderer->InitDefaultData();
      m_regenerate = false;
    }
  }

  return true;
}
} // namespace craft
