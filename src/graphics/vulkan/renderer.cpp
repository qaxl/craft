#include "renderer.hpp"

namespace craft::vk {
Renderer::Renderer()
    : m_instance({{"VK_KHR_surface"},
                  {"VK_KHR_win32_surface"},
#ifndef NDEBUG
                  {"VK_EXT_debug_utils", false}
#endif
                 },
                 {
#ifndef NDEBUG
                     "VK_LAYER_KHRONOS_validation"
#endif
                 }) {
  m_instance.SelectPhysicalDevice();
}
} // namespace craft::vk