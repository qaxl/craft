#pragma once

#include <volk.h>

namespace craft::vk {
class Instance {
public:
  Instance();

private:
  VkInstance m_instance;
};
} // namespace craft::vk
