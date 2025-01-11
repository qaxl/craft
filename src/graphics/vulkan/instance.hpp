#pragma once

#include <volk.h>

#include <initializer_list>

#include "util/optimization.hpp"

namespace craft::vk {
struct InstanceExtension {
  // the extension name to enable
  const char *name;
  // whether an extension must be loaded, or only loaded if existing
  bool required = true;
};

class Instance {
public:
  Instance(std::initializer_list<InstanceExtension> extensions);
  ~Instance();

  Instance(const Instance &) = delete;
  Instance(Instance &&) = delete;

  Instance &operator=(const Instance &) = delete;
  Instance &operator=(Instance &&) = delete;

  FORCE_INLINE VkInstance GetInstance() { return m_instance; }

private:
  VkInstance m_instance = nullptr;
  VkDebugUtilsMessengerEXT m_messenger = nullptr;
};

} // namespace craft::vk
