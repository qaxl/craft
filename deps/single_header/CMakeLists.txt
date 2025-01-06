add_library(stb STATIC stb/stb_image.c)
target_include_directories(stb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

add_library(single_header INTERFACE)
target_include_directories(single_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(single_header INTERFACE stb)
