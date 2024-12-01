#include "window.hpp"
#include "util/error.hpp"

#include <SDL3/SDL.h>

namespace craft {
Window::Window(int width, int height, const char *title) {
  m_window = SDL_CreateWindow(title, width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!m_window) {
    RuntimeError::SetErrorString(
        "Couldn't create a SDL window. Is there a display (server) available?",
        EF_AppendSDLErrors);
  }
}

Window::~Window() {
  if (m_window_is_open) {
    SDL_DestroyWindow(m_window);
  }
}

void Window::PollEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      m_window_is_open = false;
      break;
    }
  }
}
} // namespace craft
