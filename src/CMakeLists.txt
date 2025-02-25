add_executable(craft
  main.cpp
  app.cpp
  
  graphics/vulkan/device.cpp
  graphics/vulkan/imgui.cpp
  graphics/vulkan/instance.cpp
  graphics/vulkan/renderer.cpp
  graphics/vulkan/swapchain.cpp
  graphics/vulkan/texture.cpp
  graphics/vulkan/vma.cpp

  platform/tcp_socket.cpp
  platform/window.cpp
  
  util/error.cpp
  util/io.cpp
  
  world/player/player.cpp
  world/mesh.cpp
  world/world.cpp)

target_include_directories(craft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(craft PRIVATE SDL3::SDL3 GPUOpen::VulkanMemoryAllocator volk imgui ws2_32 glm single_header)

if (BUILD_WITH_STEAMSDK)
  target_link_libraries(craft PRIVATE steamworks)
endif()

add_definitions(-DGLM_FORCE_ROW_MAJOR)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
