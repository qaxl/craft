#include "window.hpp"

#include <iostream>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <imgui_impl_sdl3.h>

#include "util/error.hpp"

namespace craft {
Window::Window(int width, int height, const char *title) {
  m_window =
      SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_RELATIVE_MODE);
  if (!m_window) {
    RuntimeError::Throw("Couldn't create a SDL window. Is there a display (server) available?", EF_AppendSDLErrors);
  }

  std::cout << SDL_GetWindowRelativeMouseMode(m_window) << std::endl;
  SDL_SetWindowRelativeMouseMode(m_window, true);
}

Window::~Window() {
  if (m_window_is_open) {
    SDL_DestroyWindow(m_window);
  }
}

void Window::PollEvents() {
  SDL_Event event;
  bool spin = false;

  // Reset all per-event shit here
  m_relative_motion = std::make_pair(0.0f, 0.0f);
  m_mouse_scroll = 0.0f;

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

      case SDL_EVENT_KEY_DOWN:
        m_key_down.set(event.key.scancode, true);
        break;

      case SDL_EVENT_KEY_UP:
        m_key_down.reset(event.key.scancode);
        break;

      case SDL_EVENT_MOUSE_MOTION:
        m_relative_motion.first += event.motion.xrel;
        m_relative_motion.second += event.motion.yrel;
        break;

      case SDL_EVENT_MOUSE_WHEEL:
        m_mouse_scroll += event.wheel.y;
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        m_mouse_down.set(event.button.button);
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        m_mouse_down.reset(event.button.button);
        break;
      }

      ImGui_ImplSDL3_ProcessEvent(&event);
    }

    if (spin) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } while (spin);

  // TODO: remove
  if (IsKeyPressed(KeyboardKey::Escape)) {
    m_window_is_open = false;
  }
}
} // namespace craft
