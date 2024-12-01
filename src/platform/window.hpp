#pragma once

#include "SDL3/SDL_video.h"
#include <utility>

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
  bool IsOpen() { return m_window_is_open; }

  SDL_Window *GetHandle() { return m_window; }
  void GetSize(int &width, int &height) { SDL_GetWindowSizeInPixels(m_window, &width, &height); }

  std::pair<uint32_t, uint32_t> GetSize() {
    int width, height;
    GetSize(width, height);
    // this converts them to uint, "safely"
    return std::make_pair(width, height);
  }

private:
  SDL_Window *m_window = nullptr;
  bool m_window_is_open = true;
};
} // namespace craft
