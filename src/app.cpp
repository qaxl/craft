#include "app.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "util/error.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <SDL3/SDL.h>
#include <volk.h>

#include <iostream>

namespace craft {
App::~App() { SDL_Quit(); }

App::App() {
  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::Throw("SDL Initialization Failure", EF_AppendSDLErrors);
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::Throw(
        "Vulkan Initialization Failure: Couldn't load Vulkan library functions. Does this computer support rendering "
        "with Vulkan? Currently, we do not support graphics APIs other than Vulkan.\nOpenGL support may be added in "
        "the "
        "future for better compatibility with older hardware.\nIf you have hardware that is newer than, let's say, the "
        "last 5 to 10 years, then it most likely does support Vulkan 1.3. If that's the case, consider updating your "
        "graphics drivers.\n\nRead more for AMD graphics cards: "
        "https://www.amd.com/en/support/download/drivers.html\nRead more for NVIDIA graphics cards: "
        "https://www.nvidia.com/en-us/drivers/\n\nIf upgrading your drivers doesn't fix this issue, you most likely do "
        "not have support for Vulkan 1.3, which is the minimum required version for this application to run.\nAnd if "
        "you "
        "have the ability to program in OpenGL/DirectX 11, proficiently, you're welcome contributing into the "
        "framework that the current application is running from https://github.com/qaxl/craft :)");
  }

  m_window = std::make_shared<Window>(1024, 768, "test");
  m_renderer = std::make_shared<vk::Renderer>(m_window);

  // if (!m_socket.Connect("127.0.0.1", 62501)) {
  //   RuntimeError::Throw("Couldn't establish connection to the game server... Are you connected? If so, the game
  //   server "
  //                       "may be experiencing a temporary outage.");
  // }

  // m_socket.Send(const char *buf, size_t size);
  // m_network_thread = std::thread([&]() {});
}

bool App::Run() {
  while (m_window->IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    if (ImGui::Begin("Background Settings")) {
      auto &selected = m_renderer->GetCurrentEffect();
      static int current = 0;

      ImGui::Text("Current Effect: %s", selected.name.data());
      ImGui::SliderInt("Effect Index", &current, 0, 1);

      ImGui::InputFloat4("push constant data[0]", selected.pc.data[0].v[0]);
      ImGui::InputFloat4("push constant data[1]", selected.pc.data[1].v[0]);
      ImGui::InputFloat4("push constant data[2]", selected.pc.data[2].v[0]);
      ImGui::InputFloat4("push constant data[3]", selected.pc.data[3].v[0]);

      m_renderer->SetCurrentEffect(current);
    }
    ImGui::End();

    ImGui::Render();

    m_renderer->Draw();
    m_window->PollEvents();
  }

  return true;
}
} // namespace craft
