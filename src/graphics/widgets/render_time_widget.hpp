#pragma once

#include "widget.hpp"

#include <SDL3/SDL_timer.h>

namespace craft {
class RenderTimingsWidget : public Widget {
public:
  RenderTimingsWidget(float *render_time) : render_time(render_time) {
    m_name = "Render Timings";
    m_closable = true;
  }

  virtual void OnRender(WidgetManager *manager) override {
    uint64_t current_frame = SDL_GetTicksNS();
    uint64_t delta = current_frame - last_frame;
    last_frame = current_frame;

    float delta_s = delta / 1e9;
    float fps = 1.f / delta_s;

    ImGui::Text("FPS: %f", 1000.0f / *render_time);
    ImGui::Text("Frame Time: %fms", *render_time);
  }

private:
  uint64_t last_frame = SDL_GetTicksNS();
  float *render_time = nullptr;
};
} // namespace craft
