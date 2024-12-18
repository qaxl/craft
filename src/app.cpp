#include "app.hpp"
#include "graphics/vulkan/renderer.hpp"
#include "util/error.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>

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

  // TODO: move this somewhere
  ImPlot::CreateContext();

  // if (!m_socket.Connect("127.0.0.1", 62501)) {
  //   RuntimeError::Throw("Couldn't establish connection to the game server... Are you connected? If so, the game
  //   server "
  //                       "may be experiencing a temporary outage.");
  // }

  // m_socket.Send(const char *buf, size_t size);
  // m_network_thread = std::thread([&]() {});
}

bool App::Run() {
  uint64_t start_tick = SDL_GetTicksNS();
  uint64_t end_tick = SDL_GetTicksNS();

  while (m_window->IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    start_tick = SDL_GetTicksNS();
    float tick_difference = start_tick - end_tick;
    end_tick = start_tick;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    // ImPlot::ShowDemoWindow();

    if (ImGui::Begin("Render Statistics")) {
      float frame_time = tick_difference / 1e9f;
      ImGui::Text("Frame Time: %fms", frame_time);

      float fps = 1.f / frame_time;
      ImGui::Text("FPS: %f", fps);

      static std::vector<float> fpses;
      fpses.push_back(fps);

      if (ImPlot::BeginPlot("Frame Time Last 60 Seconds")) {
        ImPlot::SetupAxes("Time", "FPS");

        if (fpses.size() > 60 * 144) {
          ImPlot::SetupAxesLimits(fpses.size() - 60 * 144 + 10, fpses.size() + 10, 0, 200, ImPlotCond_Once);
        } else {
          ImPlot::SetupAxesLimits(0, 60 * 144, 0, 200, ImPlotCond_Once);
        }

        std::cout << fpses.size() << std::endl;

        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
        ImPlot::PlotLine("FPS", fpses.data(), fpses.size());
        ImPlot::PopStyleVar();
        ImPlot::EndPlot();
      }

      // if (fpses.size() > 60 * 144) {
      //   fpses.erase(fpses.begin());
      // }

      ImGui::End();
    }

    if (ImGui::Begin("Background Settings")) {
      auto &selected = m_renderer->GetCurrentEffect();
      static int current = 0;

      ImGui::TextWrapped(
          "This window is merely for the background settings, which is most of the time just black. But all of "
          "these other backgrounds are product of compute shaders. You can either use the Effect Index slider "
          "or select the current effect from available effects drop-down menu.");

      static bool flag = true;
      // TODO: make this actually work
      ImGui::Checkbox("Enabled", &flag);

      ImGui::NewLine();
      ImGui::NewLine();

      ImGui::Text("Current Effect: %s", selected.name.data());
      ImGui::SliderInt("Effect Index", &current, 0, 1);
      if (ImGui::BeginCombo("Effect", selected.name.data())) {
        bool is_selected = selected.name == "Gradient";
        if (ImGui::Selectable("Gradient", is_selected)) {
          m_renderer->SetCurrentEffect(0);
        }

        is_selected = selected.name == "Sky";
        if (ImGui::Selectable("Sky", is_selected)) {
          m_renderer->SetCurrentEffect(1);
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }

        ImGui::EndCombo();
      }

      ImGui::InputFloat4("push constant data[0]", selected.pc.data[0].v[0]);
      ImGui::InputFloat4("push constant data[1]", selected.pc.data[1].v[0]);
      ImGui::InputFloat4("push constant data[2]", selected.pc.data[2].v[0]);
      ImGui::InputFloat4("push constant data[3]", selected.pc.data[3].v[0]);

      // m_renderer->SetCurrentEffect(current);
    }
    ImGui::End();

    ImGui::Render();

    m_renderer->Draw();
    m_window->PollEvents();
  }

  return true;
}
} // namespace craft
