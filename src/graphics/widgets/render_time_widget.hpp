#pragma once

#include "widget.hpp"

#include <SDL3/SDL_timer.h>

namespace craft {
class RenderTimingsWidget : public Widget {
public:
  RenderTimingsWidget() {
    m_name = "Render Timings";
    m_closable = true;
  }

  virtual void OnRender(WidgetManager *manager) override {
    uint64_t current_frame = SDL_GetTicksNS();
    uint64_t delta = current_frame - last_frame;
    last_frame = current_frame;

    float delta_s = delta / 1e9;
    float fps = 1.f / delta_s;

    ImGui::Text("FPS: %f", fps);
  }

private:
  uint64_t last_frame = SDL_GetTicksNS();
};
} // namespace craft
