#pragma once

#include <format>
#include <source_location>
#include <string_view>
#include <vector>

// #define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>
// TODO: in the future, use
// [vk.xml](https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/refs/heads/main/xml/vk.xml)
// to load vulkan functions.

#include "util/error.hpp"

#define STRINGIFY(x) #x
#define STRING(x) STRINGIFY(x)
#ifdef NDEBUG
#define VK_CHECK(x) CheckResult(x)
#else
#define VK_CHECK(x) CheckResult(x, std::source_location::current())
#endif

namespace craft::vk {
static constexpr std::string_view VkResultToStr(VkResult result);

static constexpr void CheckResult(VkResult result,
#ifndef NDEBUG
                                  std::source_location location = std::source_location::current()
#endif
) {
  if (result != VK_SUCCESS) {
    RuntimeError::SetErrorString(std::format("vulkan error at {}@{}:{}: {}",
#ifdef NDEBUG
                                             "no", "debug",
                                             "info"
#else
                                             location.file_name(), location.function_name(), location.line(),
#endif
                                             VkResultToStr(result)),
                                 EF_DumpStacktrace);
  }
}

// This is probably not the best way to implement this, but it gets the job done.
template <typename T, typename F, typename R, typename... Args>
concept FallibleVkFunction = requires(F func, Args &&...args) {
  { func(std::declval<Args>()..., std::declval<uint32_t *>(), std::declval<T *>()) } -> std::convertible_to<R>;
};

template <typename T, typename F, typename... Args>
  requires FallibleVkFunction<T, F, VkResult, Args...>
static constexpr inline std::vector<T> GetProperties(F function, Args &&...args) {
  uint32_t ext_count = 0;
  VK_CHECK(function(std::forward<Args>(args)..., &ext_count, nullptr));

  std::vector<T> exts(ext_count);
  VK_CHECK(function(std::forward<Args>(args)..., &ext_count, exts.data()));

  return exts;
}

template <typename T, typename F, typename... Args>
  requires FallibleVkFunction<T, F, void, Args...>
static constexpr inline std::vector<T> GetProperties(F function, Args &&...args) {
  uint32_t ext_count = 0;
  function(std::forward<Args>(args)..., &ext_count, nullptr);

  std::vector<T> exts(ext_count);
  function(std::forward<Args>(args)..., &ext_count, exts.data());

  return exts;
}

static constexpr std::string_view VkResultToStr(VkResult result) {
  switch (result) {
  // Success Codes
  case VK_SUCCESS:
    return "VK_SUCCESS: Command successfully completed";
  case VK_NOT_READY:
    return "VK_NOT_READY: A fence or query has not yet completed";
  case VK_TIMEOUT:
    return "VK_TIMEOUT: A wait operation has not completed in the specified time";
  case VK_EVENT_SET:
    return "VK_EVENT_SET: An event is signaled";
  case VK_EVENT_RESET:
    return "VK_EVENT_RESET: An event is unsignaled";
  case VK_INCOMPLETE:
    return "VK_INCOMPLETE: A return array was too small for the result";
  case VK_SUBOPTIMAL_KHR:
    return "VK_SUBOPTIMAL_KHR: A swapchain no longer matches the surface properties exactly, but can still be used to "
           "present to the surface successfully.";
  case VK_THREAD_IDLE_KHR:
    return "VK_THREAD_IDLE_KHR: A deferred operation is not complete but there is currently no work for this thread to "
           "do at the time of this call.";
  case VK_THREAD_DONE_KHR:
    return "VK_THREAD_DONE_KHR: A deferred operation is not complete but there is no work remaining to assign to "
           "additional threads.";
  case VK_OPERATION_DEFERRED_KHR:
    return "VK_OPERATION_DEFERRED_KHR: A deferred operation was requested and at least some of the work was deferred.";
  case VK_OPERATION_NOT_DEFERRED_KHR:
    return "VK_OPERATION_NOT_DEFERRED_KHR: A deferred operation was requested and no operations were deferred.";
  case VK_PIPELINE_COMPILE_REQUIRED:
    return "VK_PIPELINE_COMPILE_REQUIRED: A requested pipeline creation would have required compilation, but the "
           "application requested compilation to not be performed.";

    // #ifdef VK_KHR_pipeline_executable_properties
    //   case VK_PIPELINE_BINARY_MISSING_KHR:
    //     return "VK_PIPELINE_BINARY_MISSING_KHR: The application attempted to create a pipeline binary by querying an
    //     "
    //            "internal cache, but the internal cache entry did not exist.";
    // #endif

#ifdef VK_EXT_shader_binary_compatibility
  case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
    return "VK_INCOMPATIBLE_SHADER_BINARY_EXT: The provided binary shader code is not compatible with this device.";
#endif

  // Error Codes
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "VK_ERROR_OUT_OF_HOST_MEMORY: A host memory allocation has failed.";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "VK_ERROR_OUT_OF_DEVICE_MEMORY: A device memory allocation has failed.";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "VK_ERROR_INITIALIZATION_FAILED: Initialization of an object could not be completed for "
           "implementation-specific reasons.";
  case VK_ERROR_DEVICE_LOST:
    return "VK_ERROR_DEVICE_LOST: The logical or physical device has been lost.";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "VK_ERROR_MEMORY_MAP_FAILED: Mapping of a memory object has failed.";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT: A requested layer is not present or could not be loaded.";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT: A requested extension is not supported.";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "VK_ERROR_FEATURE_NOT_PRESENT: A requested feature is not supported.";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER: The requested version of Vulkan is not supported by the driver or is "
           "otherwise incompatible for implementation-specific reasons.";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "VK_ERROR_TOO_MANY_OBJECTS: Too many objects of the type have already been created.";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "VK_ERROR_FORMAT_NOT_SUPPORTED: A requested format is not supported on this device.";
  case VK_ERROR_FRAGMENTED_POOL:
    return "VK_ERROR_FRAGMENTED_POOL: A pool allocation has failed due to fragmentation of the pool’s memory.";

#ifdef VK_KHR_surface
  case VK_ERROR_SURFACE_LOST_KHR:
    return "VK_ERROR_SURFACE_LOST_KHR: A surface is no longer available.";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: The requested window is already in use by Vulkan or another API in a "
           "manner which prevents it from being used again.";
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR: A surface has changed in such a way that it is no longer compatible with the "
           "swapchain.";
#endif

#ifdef VK_KHR_display
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
    return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: The display used by a swapchain does not use the same presentable image "
           "layout.";
#endif

#ifdef VK_NV_glsl_shader
  case VK_ERROR_INVALID_SHADER_NV:
    return "VK_ERROR_INVALID_SHADER_NV: One or more shaders failed to compile or link.";
#endif

#ifdef VK_KHR_external_memory
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    return "VK_ERROR_OUT_OF_POOL_MEMORY: A pool memory allocation has failed.";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    return "VK_ERROR_INVALID_EXTERNAL_HANDLE: An external handle is not a valid handle of the specified type.";
#endif

#ifdef VK_EXT_buffer_device_address
  case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
    return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: A buffer creation failed because the requested address is not "
           "available.";
#endif

#ifdef VK_EXT_full_screen_exclusive
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
    return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: Operation failed as it did not have exclusive full-screen "
           "access.";
#endif

  case VK_ERROR_UNKNOWN:
    return "VK_ERROR_UNKNOWN: An unknown error has occurred.";

  default: {
    static thread_local char buf[100]{};
    snprintf(buf, 99, "unhandled VkResult: %02x\n", result);
    return buf;
  }
  }
}

static constexpr VkRenderingAttachmentInfo
AttachmentInfo(VkImageView view, VkClearValue *clear,
               VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
  VkRenderingAttachmentInfo color_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  color_attachment.imageView = view;
  color_attachment.imageLayout = layout;
  color_attachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  if (clear) {
    color_attachment.clearValue = *clear;
  }

  return color_attachment;
}

static constexpr VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore) {
  return VkSemaphoreSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = semaphore,
      .value = 1,
      .stageMask = stage_mask,
  };
}

static constexpr VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd) {
  return VkCommandBufferSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = cmd,
  };
}

static constexpr VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signal_semaphore,
                                          VkSemaphoreSubmitInfo *wait_semaphore) {
  VkSubmitInfo2 info{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  info.waitSemaphoreInfoCount = wait_semaphore != nullptr;
  info.pWaitSemaphoreInfos = wait_semaphore;

  info.signalSemaphoreInfoCount = signal_semaphore != nullptr;
  info.pSignalSemaphoreInfos = signal_semaphore;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = cmd;

  return info;
}

} // namespace craft::vk
