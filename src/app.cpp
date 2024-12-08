#include "app.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "math/color.hpp"
#include "util/error.hpp"

#include <SDL3/SDL.h>
#include <volk.h>

#include <iostream>

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

  auto window = std::make_shared<Window>(1024, 768, "test");
  auto renderer = std::make_shared<vk::Renderer>(window);

  std::cout << colors::kWhite.v[0] << std::endl;

  return SharedState{.window = window, .renderer = renderer};
}

bool App::Run() {
  while (m_state.window->IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    m_state.renderer->Draw();
    m_state.window->PollEvents();
  }

  return true;
}
} // namespace craft
