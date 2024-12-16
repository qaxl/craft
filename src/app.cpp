#include "app.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
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

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    if (ImGui::Begin("Background Settings")) {
      auto &selected = m_state.renderer->GetCurrentEffect();
      static int current = 0;

      ImGui::Text("Current Effect: %s", selected.name.data());
      ImGui::SliderInt("Effect Index", &current, 0, 2);

      ImGui::InputFloat4("push constant data[0]", selected.pc.data[0].v[0]);
      ImGui::InputFloat4("push constant data[1]", selected.pc.data[1].v[0]);
      ImGui::InputFloat4("push constant data[2]", selected.pc.data[2].v[0]);
      ImGui::InputFloat4("push constant data[3]", selected.pc.data[3].v[0]);

      std::cout << "Current Effect: " << current << std::endl;
      m_state.renderer->SetCurrentEffect(current);
    }
    ImGui::End();

    ImGui::Render();

    m_state.renderer->Draw();
    m_state.window->PollEvents();
  }

  return true;
}
} // namespace craft
