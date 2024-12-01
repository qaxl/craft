#include "renderer.hpp"

namespace craft::vk {
Renderer::Renderer(std::shared_ptr<Window> window)
    : m_window(window), m_instance(m_window,
                                   {
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