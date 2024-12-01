add_executable(craft
  main.cpp
  app.cpp)

target_include_directories(craft PRIVATE ${CMAKE_CURRENT_SOURCE_DIRECTORY})
target_link_libraries(craft PRIVATE SDL3::SDL3 volk)
