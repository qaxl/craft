add_executable(craft
  main.cpp
  app.cpp
  
  graphics/vulkan/renderer.cpp
  graphics/vulkan/vma.cpp

  platform/window.cpp
  
  util/error.cpp)

target_include_directories(craft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(craft PRIVATE SDL3::SDL3 GPUOpen::VulkanMemoryAllocator volk)
