#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include <imgui.h>

namespace craft {
class WidgetManager;
class Widget {
public:
  virtual void OnRender(WidgetManager *manager) = 0;
  virtual ~Widget() {}

protected:
  std::string_view m_name = "Unnamed Widget";
  bool m_closable = false;

private:
  bool m_is_open = true;

  friend class WidgetManager;
};

class WidgetManager {
public:
  void AddWidget(std::unique_ptr<Widget> &&widget) { m_widgets[widget->m_name] = std::move(widget); }
  void Clear() { m_widgets.clear(); }

  void OpenWidget(std::string_view id) {
    if (auto it = m_widgets.find(id); it != m_widgets.end()) {
      it->second->m_is_open = true;
    }
  }

  const auto &GetAllWidgets() { return m_widgets; }

  void RenderWidgets() {
    for (auto &[name, widget] : m_widgets) {
      if (!widget->m_is_open)
        continue;

      if (ImGui::Begin(widget->m_name.data(), widget->m_closable ? &widget->m_is_open : nullptr)) {
        widget->OnRender(this);
      }
      ImGui::End();
    }
  }

private:
  std::unordered_map<std::string_view, std::unique_ptr<Widget>> m_widgets;
};
} // namespace craft
