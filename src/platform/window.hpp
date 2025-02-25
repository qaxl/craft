#pragma once

#include <bitset>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>

#include "keyboard.hpp"
#include "util/error.hpp"

struct SDL_Window;

namespace craft {
using WindowKeyCallback = void (*)(KeyboardKey, bool pressed);

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

  VkExtent2D GetExtent() const {
    auto [width, height] = GetSize();
    return VkExtent2D{width, height};
  }

  VkSurfaceKHR CreateSurface(VkInstance instance) {
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface)) {
      RuntimeError::Throw("Couldn't create a Vulkan surface.");
    }
    return surface;
  }

  bool IsKeyPressed(KeyboardKey key) { return m_key_down.test(static_cast<size_t>(key)); }
  bool IsButtonPressed(uint8_t button) {
    bool is = m_mouse_down.test(button);
    m_mouse_down.reset(button);
    return is;
  }

  std::pair<float, float> GetRelativeMouseMotion() { return m_relative_motion; }
  float GetMouseScroll() { return m_mouse_scroll; }

  void ToggleRelativeMouseMode() {
    SDL_SetWindowRelativeMouseMode(m_window, !SDL_GetWindowRelativeMouseMode(m_window));
  }

  void OnKeyEvent(WindowKeyCallback cb) { m_key_callbacks.emplace_back(cb); }

private:
  SDL_Window *m_window = nullptr;
  bool m_window_is_open = true;

  std::bitset<SDL_SCANCODE_COUNT> m_key_down;
  std::bitset<64> m_mouse_down;
  std::pair<float, float> m_relative_motion = std::make_pair(0.0f, 0.0f);
  float m_mouse_scroll = 0.0f;

  std::vector<WindowKeyCallback> m_key_callbacks;
};
} // namespace craft
