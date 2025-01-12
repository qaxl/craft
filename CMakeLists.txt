cmake_minimum_required(VERSION 3.27)
project(craft)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=haswell")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
endif()

option(BUILD_WITH_STEAMSDK "Build with Steamworks SDK" OFF)

add_subdirectory(src)
add_subdirectory(deps)
add_subdirectory(shaders)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Enable memory checks
# if (MSVC)
#     target_compile_options(craft PRIVATE /fsanitize=address)
#     target_link_options(craft PRIVATE /fsanitize=address)
# else()
#     target_compile_options(craft PRIVATE -fsanitize=address -shared-libsan)
#     target_link_options(craft PRIVATE -fsanitize=address -shared-libsan)
# endif()
