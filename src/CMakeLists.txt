add_executable(craft
  main.cpp)

if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  set(PLATFORM_LIBRARIES X11::xcb)
else()
  message(FATAL_ERROR "Platform is unsupported. Currently only supported platform is Linux (X11). Linux (Wayland) and any other platform is not supported.")
endif()

target_include_directories(craft PRIVATE ${CMAKE_CURRENT_SOURCE_DIRECTORY})
target_link_libraries(craft PRIVATE volk ${PLATFORM_LIBRARIES})
