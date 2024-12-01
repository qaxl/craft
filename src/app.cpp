#include "app.hpp"

#include <SDL3/SDL.h>
#include <volk.h>

namespace craft {
static std::string s_global_error_description;

App::~App() { SDL_Quit(); }

std::optional<App> App::Init() {
  App app;

  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    s_global_error_description.append("SDL Initialization Failure:\n", 28);

    // Truncated SDL error output.
    const char *error = SDL_GetError();
    size_t error_len = strlen(error);
    if (error_len > (s_global_error_description.capacity() -
                     s_global_error_description.size()))
      error_len = (s_global_error_description.capacity() -
                   s_global_error_description.size());

    s_global_error_description.append(error, error_len);

    return std::nullopt;
  }

  if (volkInitialize() != VK_SUCCESS) {
    static const char error[] =
        "Vulkan Initialization Failure: couldn't load vulkan library "
        "functions. Does this computer support rendering with Vulkan? "
        "Currently we do not support other graphics APIs other than Vulkan; "
        "OpenGL support may be added in the future for better compability with "
        "older hardware.";
    size_t error_len = sizeof(error) / sizeof(*error);

    s_global_error_description.append(error, error_len);
  }

  // Will preallocate here, if the program errors out due to memory
  // constraints, then our error handling code won't crash the program.
  // WARNING: If the message is longer than 4096 (ASCII) characters, it will
  // get truncated.
  s_global_error_description.reserve(4096);

  app.m_state = std::make_shared<SharedState>();

  return app;
}

std::string_view App::GetErrorString() { return s_global_error_description; }

void App::Run() {}
} // namespace craft
