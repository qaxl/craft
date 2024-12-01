#pragma once

#include <string_view>

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk.h>
// TODO: in the future, use
// [vk.xml](https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/refs/heads/main/xml/vk.xml)
// to load vulkan functions.

#include "util/error.hpp"

#define VKCHECK(x) CheckResult(x, #x)

namespace craft::vk {
static std::string_view VkResultToStr(VkResult result);

static void CheckResult(VkResult result, std::string_view function = "") {
  if (result != VK_SUCCESS) {
    RuntimeError::SetErrorString(std::format("vulkan function {} error: {}",
                                             function, VkResultToStr(result)));
  }
}

static std::string_view VkResultToStr(VkResult result) {
  switch (result) {
  // Success Codes
  case VK_SUCCESS:
    return "VK_SUCCESS: Command successfully completed";
  case VK_NOT_READY:
    return "VK_NOT_READY: A fence or query has not yet completed";
  case VK_TIMEOUT:
    return "VK_TIMEOUT: A wait operation has not completed in the specified "
           "time";
  case VK_EVENT_SET:
    return "VK_EVENT_SET: An event is signaled";
  case VK_EVENT_RESET:
    return "VK_EVENT_RESET: An event is unsignaled";
  case VK_INCOMPLETE:
    return "VK_INCOMPLETE: A return array was too small for the result";
  case VK_SUBOPTIMAL_KHR:
    return "VK_SUBOPTIMAL_KHR: A swapchain no longer matches the surface "
           "properties exactly, but can still be used to present to the "
           "surface successfully.";
  case VK_THREAD_IDLE_KHR:
    return "VK_THREAD_IDLE_KHR: A deferred operation is not complete but there "
           "is currently no work for this thread to do at the time of this "
           "call.";
  case VK_THREAD_DONE_KHR:
    return "VK_THREAD_DONE_KHR: A deferred operation is not complete but there "
           "is no work remaining to assign to additional threads.";
  case VK_OPERATION_DEFERRED_KHR:
    return "VK_OPERATION_DEFERRED_KHR: A deferred operation was requested and "
           "at least some of the work was deferred.";
  case VK_OPERATION_NOT_DEFERRED_KHR:
    return "VK_OPERATION_NOT_DEFERRED_KHR: A deferred operation was requested "
           "and no operations were deferred.";
  case VK_PIPELINE_COMPILE_REQUIRED:
    return "VK_PIPELINE_COMPILE_REQUIRED: A requested pipeline creation would "
           "have required compilation, but the application requested "
           "compilation to not be performed.";
#ifdef VK_PIPELINE_BINARY_MISSING_KHR
  case VK_PIPELINE_BINARY_MISSING_KHR:
    return "VK_PIPELINE_BINARY_MISSING_KHR: The application attempted to "
           "create a pipeline binary by querying an internal cache, but the "
           "internal cache entry did not exist.";
#endif
#ifdef VK_INCOMPATIBLE_SHADER_BINARY_EXT
  case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
    return "VK_INCOMPATIBLE_SHADER_BINARY_EXT: The provided binary shader code "
           "is not compatible with this device.";
#endif

  // Error Codes
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "VK_ERROR_OUT_OF_HOST_MEMORY: A host memory allocation has failed.";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "VK_ERROR_OUT_OF_DEVICE_MEMORY: A device memory allocation has "
           "failed.";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "VK_ERROR_INITIALIZATION_FAILED: Initialization of an object could "
           "not be completed for implementation-specific reasons.";
  case VK_ERROR_DEVICE_LOST:
    return "VK_ERROR_DEVICE_LOST: The logical or physical device has been "
           "lost.";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "VK_ERROR_MEMORY_MAP_FAILED: Mapping of a memory object has failed.";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT: A requested layer is not present or "
           "could not be loaded.";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT: A requested extension is not "
           "supported.";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "VK_ERROR_FEATURE_NOT_PRESENT: A requested feature is not "
           "supported.";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER: The requested version of Vulkan is "
           "not supported by the driver or is otherwise incompatible for "
           "implementation-specific reasons.";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "VK_ERROR_TOO_MANY_OBJECTS: Too many objects of the type have "
           "already been created.";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "VK_ERROR_FORMAT_NOT_SUPPORTED: A requested format is not supported "
           "on this device.";
  case VK_ERROR_FRAGMENTED_POOL:
    return "VK_ERROR_FRAGMENTED_POOL: A pool allocation has failed due to "
           "fragmentation of the pool’s memory.";
#ifdef VK_ERROR_SURFACE_LOST_KHR
  case VK_ERROR_SURFACE_LOST_KHR:
    return "VK_ERROR_SURFACE_LOST_KHR: A surface is no longer available.";
#endif
#ifdef VK_ERROR_NATIVE_WINDOW_IN_USE_KHR
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: The requested window is already "
           "in use by Vulkan or another API in a manner which prevents it from "
           "being used again.";
#endif
#ifdef VK_ERROR_OUT_OF_DATE_KHR
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR: A surface has changed in such a way that "
           "it is no longer compatible with the swapchain.";
#endif
#ifdef VK_ERROR_INCOMPATIBLE_DISPLAY_KHR
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
    return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: The display used by a swapchain "
           "does not use the same presentable image layout.";
#endif
#ifdef VK_ERROR_INVALID_SHADER_NV
  case VK_ERROR_INVALID_SHADER_NV:
    return "VK_ERROR_INVALID_SHADER_NV: One or more shaders failed to compile "
           "or link.";
#endif
#ifdef VK_ERROR_OUT_OF_POOL_MEMORY
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    return "VK_ERROR_OUT_OF_POOL_MEMORY: A pool memory allocation has failed.";
#endif
#ifdef VK_ERROR_INVALID_EXTERNAL_HANDLE
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    return "VK_ERROR_INVALID_EXTERNAL_HANDLE: An external handle is not a "
           "valid handle of the specified type.";
#endif
#ifdef VK_ERROR_FRAGMENTATION
  case VK_ERROR_FRAGMENTATION:
    return "VK_ERROR_FRAGMENTATION: A descriptor pool creation has failed due "
           "to fragmentation.";
#endif
#ifdef VK_ERROR_INVALID_DEVICE_ADDRESS_EXT
  case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
    return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: A buffer creation failed "
           "because the requested address is not available.";
#endif
#ifdef VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS
  case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
    return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: A buffer creation or "
           "memory allocation failed due to address unavailability.";
#endif
#ifdef VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
    return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: Operation failed as "
           "it did not have exclusive full-screen access.";
#endif
#ifdef VK_ERROR_VALIDATION_FAILED_EXT
  case VK_ERROR_VALIDATION_FAILED_EXT:
    return "VK_ERROR_VALIDATION_FAILED_EXT: Command failed because invalid "
           "usage was detected.";
#endif
#ifdef VK_ERROR_COMPRESSION_EXHAUSTED_EXT
  case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
    return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT: Image creation failed due to "
           "exhaustion of internal resources required for compression.";
#endif
#ifdef VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR
  case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
    return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: The requested "
           "VkImageUsageFlags are not supported.";
#endif
#ifdef VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR
  case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: The requested "
           "video picture layout is not supported.";
#endif
#ifdef VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR
  case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: The video "
           "profile operation is not supported.";
#endif
#ifdef VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR
  case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: Format parameters "
           "in a requested video profile chain are not supported.";
#endif
#ifdef VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR
  case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: Codec-specific "
           "parameters are not supported.";
#endif
#ifdef VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR
  case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: The specified video "
           "Std header version is not supported.";
#endif
#ifdef VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR
  case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
    return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: The specified Video Std "
           "parameters are invalid.";
#endif
#ifdef VK_ERROR_NOT_PERMITTED_KHR
  case VK_ERROR_NOT_PERMITTED_KHR:
    return "VK_ERROR_NOT_PERMITTED_KHR: The driver implementation has denied a "
           "request for priority access.";
#endif
#ifdef VK_ERROR_NOT_ENOUGH_SPACE_KHR
  case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
    return "VK_ERROR_NOT_ENOUGH_SPACE_KHR: The application did not provide "
           "enough space to return all the required data.";
#endif
  case VK_ERROR_UNKNOWN:
    return "VK_ERROR_UNKNOWN: An unknown error has occurred.";

  default:
    return "Unknown VkResult";
  }
}
} // namespace craft::vk
