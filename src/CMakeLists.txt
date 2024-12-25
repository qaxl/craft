add_executable(craft
  main.cpp
  app.cpp
  
  graphics/vulkan/device.cpp
  graphics/vulkan/imgui.cpp
  graphics/vulkan/renderer.cpp
  graphics/vulkan/swapchain.cpp
  graphics/vulkan/vma.cpp

  platform/tcp_socket.cpp
  platform/window.cpp
  
  util/error.cpp)

target_include_directories(craft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(craft PRIVATE SDL3::SDL3 GPUOpen::VulkanMemoryAllocator volk glad imgui ws2_32)

if (BUILD_WITH_STEAMSDK)
  target_link_libraries(craft PRIVATE steamworks)
endif()
