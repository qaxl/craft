#pragma once

#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>

#include "util/error.hpp"

struct SDL_Window;

namespace craft {
class Window {
public:
  Window(int width, int height, const char *title = "craft engine window");
  ~Window();

  Window(const Window &) = delete;
  Window(Window &&window) {
    m_window = window.m_window;
    m_window_is_open = window.m_window_is_open;

    window.m_window_is_open = false;
    window.m_window = nullptr;
  }

  void PollEvents();
  bool IsOpen() const { return m_window_is_open; }

  SDL_Window *GetHandle() { return m_window; }
  void GetSize(int &width, int &height) const { SDL_GetWindowSizeInPixels(m_window, &width, &height); }

  std::pair<uint32_t, uint32_t> GetSize() const {
    int width, height;
    GetSize(width, height);
    // this converts them to uint, "safely"
    return std::make_pair(width, height);
  }

  VkSurfaceKHR CreateSurface(VkInstance instance) {
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface)) {
      RuntimeError::Throw("Couldn't create a Vulkan surface.");
    }
    return surface;
  }

private:
  SDL_Window *m_window = nullptr;
  bool m_window_is_open = true;
};
} // namespace craft
