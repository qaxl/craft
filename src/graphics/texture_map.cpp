#include "texture_map.hpp"

#include <stb/stb_image.h>

#include <cstdlib>
#include <iostream>

namespace craft {
TextureMap::TextureMap(const char *path) {
  int ch = 4;
  m_pixel_data.pixel_data = stbi_load(path, &m_pixel_data.width, &m_pixel_data.height, &ch, 4);
  if (!m_pixel_data.pixel_data) {
    m_pixel_data.width = m_pixel_data.height = 16;
    m_pixel_data.pixel_data = static_cast<uint8_t *>(calloc(m_pixel_data.width * m_pixel_data.height, 4));

    for (int i = 0; i < m_pixel_data.width * m_pixel_data.height; ++i) {
      reinterpret_cast<uint32_t *>(m_pixel_data.pixel_data)[i] = 0x800080FF;
      // TODO: logging system
      std::cout << "Couldn't locate texture map: " << path << ", using default texture for it." << std::endl;
    }
  }
}

TextureMap::~TextureMap() {}
} // namespace craft
