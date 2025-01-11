#include "app.hpp"

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

static glm::vec3 cast_a_ray(Window *window, Camera *camera) {
  auto [width, height] = window->GetSize();

  // Normalize mouse coordinates to NDC (-1 to 1 range)
  float x = (2.0f * static_cast<float>(width) / 2.0f) / static_cast<float>(width) - 1.0f;
  float y = 1.0f - (2.0f * static_cast<float>(height) / 2.0f) / static_cast<float>(height);

  // Unproject the screen-space point to world space
  glm::mat4 proj_view =
      glm::infinitePerspectiveRH_ZO(camera->GetFov(), static_cast<float>(width) / static_cast<float>(height), 0.1f) *
      camera->ViewMatrix();
  glm::mat4 inv = glm::inverse(proj_view);

  if (glm::determinant(proj_view) == 0.0f) {
    std::cerr << "Warning: Projection * View matrix is singular!" << std::endl;
    return glm::vec3(0.0f);
  }

  glm::vec4 nearPoint = glm::vec4(x, y, -1.0f, 1.0f); // Near plane is at z = -1

  // Transform the near point to world space
  glm::vec4 nearWorld = inv * nearPoint;
  if (nearWorld.w == 0.0f) {
    std::cerr << "Warning: Near point has zero w component!" << std::endl;
    return glm::vec3(0.0f);
  }
  nearWorld /= nearWorld.w; // Homogenize the point

  // The far point concept is removed for infinite perspective, so we just get the direction
  glm::vec3 rayDir = glm::normalize(glm::vec3(nearWorld)); // Use the direction directly

  return rayDir;
}

bool intersect_ray_aabb(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, const glm::vec3 &aabbMin,
                        const glm::vec3 &aabbMax, float tMin, float tMax) {
  float t1, t2;

  // Initialize tMin and tMax to -infinity and +infinity
  tMin = -std::numeric_limits<float>::infinity();
  tMax = std::numeric_limits<float>::infinity();

  // Check for intersection with each axis (X, Y, Z)
  for (int i = 0; i < 3; i++) {
    // Calculate the entry and exit times for the ray on this axis
    if (rayDir[i] != 0.0f) {
      t1 = (aabbMin[i] - rayOrigin[i]) / rayDir[i];
      t2 = (aabbMax[i] - rayOrigin[i]) / rayDir[i];

      // Ensure t1 is the smaller value and t2 is the larger value
      if (t1 > t2)
        std::swap(t1, t2);

      // Update the overall tMin and tMax values for this axis
      tMin = std::max(tMin, t1);
      tMax = std::min(tMax, t2);

      // If tMax is less than tMin, no intersection (the ray misses the box)
      if (tMax < tMin) {
        return false;
      }
    } else {
      // If rayDir[i] == 0, we check if the ray is outside the box along this axis
      if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i]) {
        return false;
      }
    }
  }

  // The ray intersects the AABB if tMin is less than or equal to tMax
  return tMax >= tMin;
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
        auto v = cast_a_ray(m_window.get(), &m_camera);
        std::cout << "Ray Direction: (" << v.x << ", " << v.y << ", " << v.z << ")\n";

        // FIXME: this is extremely slow
        for (int z = 0; z < kMaxChunkDepth; ++z) {
          for (int x = 0; x < kMaxChunkWidth; ++x) {
            for (int y = 0; y < kMaxChunkHeight; ++y) {
              if (m_chunk->blocks[z][x][y].block_type == BlockType::Air) {
                continue;
              }

              glm::vec3 block_position = glm::vec3(-x, -y, -z);
              glm::vec3 block_size = glm::vec3(1.0f);

              if (intersect_ray_aabb(m_camera.GetPosition(), v, block_position, block_position + block_size, 0.0f,
                                     0.0f)) {
                std::cout << "Deleting a block at (" << x << ", " << y << ", " << z << ")\n";
                m_chunk->blocks[z][x][y].block_type = BlockType::Lava;
                m_renderer->InitDefaultData();
                break;
              }
            }
          }
        }
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
