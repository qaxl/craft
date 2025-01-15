#include "app.hpp"
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
App::~App() {
  SDL_Quit();
  // SteamAPI_Shutdown();
}

App::App() : m_world{&m_noise} {
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

      if (m_window->IsKeyPressed(KeyboardKey::PageUp)) {
        m_current_block_type = static_cast<BlockType>(static_cast<int>(m_current_block_type) + 1) == BlockType::Count
                                   ? BlockType::Air
                                   : static_cast<BlockType>(static_cast<int>(m_current_block_type) + 1);
      }

      if (m_window->IsButtonPressed(1)) {

        glm::vec3 closest_block_position;
        float closest_t = std::numeric_limits<float>::max();
        bool found_intersection = false;

        for (int z = 0; z < kMaxChunkDepth; ++z) {
          for (int x = 0; x < kMaxChunkWidth; ++x) {
            for (int y = 0; y < kMaxChunkHeight; ++y) {
              if (m_chunk->blocks[z][x][y].block_type == BlockType::Air) {
                continue;
              }

              // Calculate block position and size
              glm::vec3 block_position = glm::vec3(-x, -y, -z);
              glm::vec3 block_min = block_position;
              glm::vec3 block_max = block_position + glm::vec3(1.0f); // Assuming blocks are 1x1x1 in size

              AABB block_aabb{block_min, block_max};

              float t;
              // Check for intersection with the block's AABB
              if (CastRayFromCamera(m_camera, block_aabb, t)) {
                if (t < closest_t) {
                  closest_t = t;
                  closest_block_position = block_position;
                  found_intersection = true;
                }
              }
            }
          }
        }

        if (found_intersection) {
          int bx = static_cast<int>(-closest_block_position.x);
          int by = static_cast<int>(-closest_block_position.y);
          int bz = static_cast<int>(-closest_block_position.z);

          if (bx >= 0 && bx < kMaxChunkWidth && by >= 0 && by < kMaxChunkHeight && bz >= 0 && bz < kMaxChunkDepth) {
            std::cout << "Block position: " << bx << ", " << by << ", " << bz << std::endl;
            if (m_replace) {
              // Replace the block
              m_chunk->blocks[bz][bx][by].block_type = m_current_block_type;
            } else {
              // Add a block in front
              glm::vec3 new_block_pos = closest_block_position - glm::normalize(m_camera.GetForward());

              std::cout << "Closest block position: " << closest_block_position.x << ", " << closest_block_position.y
                        << ", " << closest_block_position.z << std::endl;
              std::cout << "Placing a block at: " << new_block_pos.x << ", " << new_block_pos.y << ", "
                        << new_block_pos.z << std::endl;

              int nx = static_cast<int>(-new_block_pos.x);
              int ny = static_cast<int>(-new_block_pos.y);
              int nz = static_cast<int>(-new_block_pos.z);

              if (nx >= 0 && nx < kMaxChunkWidth && ny >= 0 && ny < kMaxChunkHeight && nz >= 0 && nz < kMaxChunkDepth) {
                m_chunk->blocks[nz][nx][ny].block_type = m_current_block_type;
              }
            }

            // Reinitialize data if needed
            m_renderer->InitDefaultData();
          } else {
            std::cerr << "Block position out of bounds: " << closest_block_position.x << ", "
                      << closest_block_position.y << ", " << closest_block_position.z << std::endl;
          }
        }
        // if (found_intersection) {
        //   if (m_replace) {
        //     m_chunk
        //         ->blocks[static_cast<int>(-closest_block_position.z)][static_cast<int>(-closest_block_position.x)]
        //                 [static_cast<int>(-closest_block_position.y)]
        //         .block_type = m_current_block_type;
        //   } else {
        //     closest_block_position += glm::normalize(m_camera.GetForward());
        //     closest_block_position = glm::floor(closest_block_position + 0.5f); // Snap to nearest integer position

        //     int bx = static_cast<int>(-closest_block_position.x);
        //     int by = static_cast<int>(-closest_block_position.y);
        //     int bz = static_cast<int>(-closest_block_position.z);

        //     if (bx >= 0 && bx < kMaxChunkWidth && by >= 0 && by < kMaxChunkHeight && bz >= 0 && bz < kMaxChunkDepth)
        //     {
        //       m_chunk->blocks[bz][bx][by].block_type = m_current_block_type;
        //     } else {
        //       std::cerr << "Position out of bounds: " << closest_block_position.x << ", " << closest_block_position.y
        //                 << ", " << closest_block_position.z << std::endl;
        //     }

        //     m_chunk
        //         ->blocks[static_cast<int>(-closest_block_position.z)][static_cast<int>(-closest_block_position.x)]
        //                 [static_cast<int>(-closest_block_position.y)]
        //         .block_type = m_current_block_type;
        //   }
        //   // Reinitialize data if needed
        //   m_renderer->InitDefaultData();

        //   // // Break out of nested loops
        //   // z = kMaxChunkDepth; // Skip remaining checks
        //   // x = kMaxChunkWidth;
        //   // y = kMaxChunkHeight;
        // }
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
