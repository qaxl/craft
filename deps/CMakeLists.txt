add_subdirectory(volk)
add_subdirectory(SDL)
add_subdirectory(VulkanMemoryAllocator)
add_subdirectory(glad)

# ImGui
add_library(imgui 
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui.cpp

    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    
    # ImPlot (extension to ImGui)
    implot/implot_demo.cpp
    implot/implot_items.cpp
    implot/implot.cpp)

target_link_libraries(imgui SDL3::SDL3 volk)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends ${CMAKE_CURRENT_SOURCE_DIR}/implot)
