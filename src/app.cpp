#include "app.hpp"
#include "SDL3/SDL_video.h"
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL

#include <SDL3/SDL.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>
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

App::App(int argc, char **argv) : m_world{&m_noise} {
  ParseParameters(argc, argv);

  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::Throw("SDL Initialization Failure", EF_AppendSDLErrors);
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::Throw("Vulkan Initialization Failure. Try updating your drivers. This application requires Vulkan to "
                        "operate, and must quit now.");
  }

  m_chunk = std::make_unique<Chunk>();
  GenerateChunk(false, 10.0f, m_noise, *m_chunk);

  m_world.Generate();

  m_window = std::make_shared<Window>(1024, 768, "test");
  m_renderer = std::make_shared<vk::Renderer>(m_window, m_camera, &m_world);

  m_widget_manager = std::make_shared<WidgetManager>();
  m_widget_manager->AddWidget(std::make_unique<UtilWidget>());
  m_widget_manager->AddWidget(std::make_unique<RenderTimingsWidget>(&time_taken_to_render));
  m_widget_manager->AddWidget(std::make_unique<TerrainWidget>(m_regenerate, m_noise, m_regenerate_with_one_block,
                                                              m_scale_factor, m_max_height, m_current_block_type,
                                                              m_replace));
}

// Ray structure
struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

// AABB structure for the cube
struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

// Function to check ray-AABB intersection
bool RayIntersectsAABB(const Ray &ray, const AABB &box, float &t) {
  float tmin = (box.min.x - ray.origin.x) / ray.direction.x;
  float tmax = (box.max.x - ray.origin.x) / ray.direction.x;

  if (tmin > tmax)
    std::swap(tmin, tmax);

  float tymin = (box.min.y - ray.origin.y) / ray.direction.y;
  float tymax = (box.max.y - ray.origin.y) / ray.direction.y;

  if (tymin > tymax)
    std::swap(tymin, tymax);

  if ((tmin > tymax) || (tymin > tmax))
    return false;

  if (tymin > tmin)
    tmin = tymin;
  if (tymax < tmax)
    tmax = tymax;

  float tzmin = (box.min.z - ray.origin.z) / ray.direction.z;
  float tzmax = (box.max.z - ray.origin.z) / ray.direction.z;

  if (tzmin > tzmax)
    std::swap(tzmin, tzmax);

  if ((tmin > tzmax) || (tzmin > tmax))
    return false;

  if (tzmin > tmin)
    tmin = tzmin;
  if (tzmax < tmax)
    tmax = tzmax;

  t = tmin; // Closest intersection point
  return true;
}

// Function to cast a ray from the camera and check intersection
static bool CastRayFromCamera(const Camera &camera, const AABB &box, float &t) {
  Ray ray;
  ray.origin = camera.GetPosition();
  ray.direction = glm::normalize(camera.GetForward()); // Ensure the direction is normalized

  return RayIntersectsAABB(ray, box, t);
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
    frame_60_average_render_time += time_taken;
    frames += 1;
    if (frames == 59) {
      frames = 0;
      uint64_t time = frame_60_average_render_time / 60ULL;
      frame_60_average_render_time = 0;
      time_taken_to_render = time / 1000.0f / 1000.0f;
    }

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

      if (m_window->IsKeyPressed(KeyboardKey::PageUp)) {
        m_current_block_type = static_cast<BlockType>(static_cast<int>(m_current_block_type) + 1) == BlockType::Count
                                   ? BlockType::Air
                                   : static_cast<BlockType>(static_cast<int>(m_current_block_type) + 1);
      }

      if (m_window->IsKeyPressed(KeyboardKey::LCtrl)) {
        m_camera.IncreaseMovementSpeedBy(5.0f);
      }

      if (m_window->IsButtonPressed(1)) {
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
