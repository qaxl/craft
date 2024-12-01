#pragma once

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

private:
  SDL_Window *m_window = nullptr;
  bool m_window_is_open = true;
};
} // namespace craft
