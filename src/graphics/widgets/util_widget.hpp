#pragma once

#include "widget.hpp"

namespace craft {
class UtilWidget : public Widget {
public:
  UtilWidget() {
    m_name = "Utils";
    m_closable = false;
  }

  virtual void OnRender(WidgetManager *manager) override {
    ImGui::TextWrapped(
        "This is an util widget. It's main purpose is to open other widget windows. Not all widget windows are "
        "closable, such as this one, but they're listed there anyway.");

    for (const auto &[name, widget] : manager->GetAllWidgets()) {
      if (ImGui::Button(name.data())) {
        manager->OpenWidget(name);
      }
    }
  }

private:
};
} // namespace craft
