#include "window.hpp"

#include <thread>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>

#include "util/error.hpp"

namespace craft {
Window::Window(int width, int height, const char *title) {
  m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!m_window) {
    RuntimeError::Throw("Couldn't create a SDL window. Is there a display (server) available?", EF_AppendSDLErrors);
  }
}

Window::~Window() {
  if (m_window_is_open) {
    SDL_DestroyWindow(m_window);
  }
}

void Window::PollEvents() {
  SDL_Event event;
  bool spin = false;

  do {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        m_window_is_open = false;
        break;

      case SDL_EVENT_WINDOW_MINIMIZED:
        spin = true;
        break;

      case SDL_EVENT_WINDOW_RESTORED:
        spin = false;
        break;
      }

      ImGui_ImplSDL3_ProcessEvent(&event);
    }

    if (spin) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } while (spin);
}
} // namespace craft
