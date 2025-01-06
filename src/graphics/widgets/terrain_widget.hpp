#pragma once

#include "imgui.h"
#include "widget.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

namespace craft {
class TerrainWidget : public Widget {
public:
  TerrainWidget(bool &regenerate, FastNoiseLite &noise) : m_regenerate{regenerate}, m_noise{noise} {
    m_name = "Terrain";
    m_closable = false;
  }

  virtual void OnRender(WidgetManager *manager) override {
    static const char *items[6] = {"OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value"};
    ImGui::Combo("Noise Type", &m_current_item, items, 6);
    ImGui::InputInt("Seed", &m_seed);

    if (ImGui::Button("Generate Chunk")) {
      m_noise.SetSeed(m_seed);
      m_noise.SetNoiseType((FastNoiseLite::NoiseType)m_current_item);
      m_regenerate = true;
    }
  }

private:
  bool &m_regenerate;
  FastNoiseLite &m_noise;
  int m_current_item = 0;
  int m_seed = 0;
};
} // namespace craft