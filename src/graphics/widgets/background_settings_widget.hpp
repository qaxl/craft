#pragma once

#include "widget.hpp"

#include "graphics/vulkan/renderer.hpp"

namespace craft {
class BackgroundSettingsWidget : public Widget {
public:
  BackgroundSettingsWidget(std::shared_ptr<vk::Renderer> renderer) : m_renderer{renderer} {
    m_name = "Background Settings";
    m_closable = true;
  }

  virtual void OnRender(WidgetManager *manager) override {
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
  }

private:
  std::shared_ptr<vk::Renderer> m_renderer;
};
} // namespace craft
