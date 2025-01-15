#pragma once

#include "imgui.h"
#include "widget.hpp"
#include "world/chunk.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

namespace craft {
class TerrainWidget : public Widget {
public:
  TerrainWidget(bool &regenerate, FastNoiseLite &noise, bool &regenerate_with_one_block, float &scale_factor,
                float &max_height, BlockType &block_type, bool &replace)
      : m_regenerate{regenerate}, m_noise{noise}, m_regenerate_with_one_block{regenerate_with_one_block},
        m_scale_factor{scale_factor}, m_max_height{max_height}, m_block_type{block_type}, m_replace{replace} {
    m_name = "Terrain";
    m_closable = true;
  }

  virtual void OnRender(WidgetManager *manager) override {
    static const char *items[6] = {"OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value"};
    ImGui::Combo("Noise Type", &m_current_item, items, IM_ARRAYSIZE(items));

    static const char *fractal_items[] = {
        "None", "FBm", "Ridged", "PingPong", "DomainWarpProgressive", "DomainWarpIndependent"};
    ImGui::Combo("Fractal Type", (int *)&m_fractal_type, fractal_items, IM_ARRAYSIZE(fractal_items));

    // Add other fractal options to it too.
    ImGui::SliderInt("Fractal Octaves", &m_octaves, 1, 10);
    m_noise.SetFractalOctaves(m_octaves);

    ImGui::SliderFloat("Fractal Lacunarity", &m_lacunarity, 0.1f, 5.0f);
    m_noise.SetFractalLacunarity(m_lacunarity);

    ImGui::SliderFloat("Fractal Gain", &m_gain, 0.1f, 2.0f);
    m_noise.SetFractalGain(m_gain);

    ImGui::InputInt("Seed", &m_seed);
    ImGui::InputFloat("Scale Factor", &m_scale_factor);
    ImGui::InputFloat("Max Height", &m_max_height);
    ImGui::Checkbox("Only generate a single block (only useful for testing if a mesh is drawn successfully)",
                    &m_regenerate_with_one_block);
    ImGui::NewLine();

    int block_type = static_cast<int>(m_block_type);
    ImGui::Combo("Block Type", (int *)&block_type, "Air\0Dirt\0Lava\0");
    m_block_type = static_cast<BlockType>(block_type);

    ImGui::SameLine();
    ImGui::Checkbox("Replace Mode", &m_replace);

    if (ImGui::Button("Generate Chunk")) {
      m_noise.SetSeed(m_seed);
      m_noise.SetNoiseType((FastNoiseLite::NoiseType)m_current_item);
      m_noise.SetFractalType(m_fractal_type);
      m_noise.SetFractalOctaves(m_octaves);
      m_noise.SetFractalLacunarity(m_lacunarity);
      m_noise.SetFractalGain(m_gain);
      m_regenerate = true;
    }
  }

private:
  bool &m_regenerate;
  FastNoiseLite &m_noise;

  int m_current_item = rand();
  FastNoiseLite::FractalType m_fractal_type = FastNoiseLite::FractalType_None;
  int m_octaves = 3;
  float m_lacunarity = 2.0f;
  float m_gain = 0.5f;

  int m_seed = 0;
  bool &m_regenerate_with_one_block;
  float &m_scale_factor;
  float &m_max_height;
  BlockType &m_block_type;
  bool &m_replace;
};
} // namespace craft