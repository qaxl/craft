#include "app.hpp"
#include "util/error.hpp"

#include <SDL3/SDL.h>
#include <volk.h>

namespace craft {
App::~App() { SDL_Quit(); }

std::optional<App> App::Init() {
  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::SetErrorString("SDL Initialization Failure", EF_AppendSDLErrors);

    return std::nullopt;
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::SetErrorString("Vulkan Initialization Failure: couldn't load vulkan library "
                                 "functions. Does this computer support rendering with Vulkan? "
                                 "Currently we do not support other graphics APIs other than Vulkan; "
                                 "OpenGL support may be added in the future for better compability with "
                                 "older hardware.");

    return std::nullopt;
  }

  auto window = Window(1024, 768, "test");

  return SharedState{.window = std::move(window)};
}

bool App::Run() {
  while (m_state.window.IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    m_state.window.PollEvents();
  }

  return true;
}
} // namespace craft
