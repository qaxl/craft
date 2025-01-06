#pragma once

#include <cstdint>
#include <cstdlib>

namespace craft {
struct TextureMapPixelData {
  int width{}, height{};
  uint8_t *pixel_data{};
};

class TextureMap {
public:
  TextureMap(const char *data);
  ~TextureMap();

  TextureMapPixelData GetPixelData() { return m_pixel_data; }
  void ReleasePixelData() { free(m_pixel_data.pixel_data); }

private:
  TextureMapPixelData m_pixel_data;
};
} // namespace craft
