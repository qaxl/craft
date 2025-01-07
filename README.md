# Craft

![Screenshot of generated chunk](/.github/screenshots/screenshot.png)

Custom Voxel Engine in C++ with Vulkan

> [!NOTE]  
> Craft is still under heavy development. Do not expect a functional product in its current state.

## Features

| Feature                                               | Implemented? |
| ----------------------------------------------------- | ------------ |
| Implement Vulkan abstractions for most things         | YES          |
| Implement voxel generation and meshing                | MOSTLY DONE  |
| Implement world editing (building and breaking)       | NO           |
| Play as rectangles that shoot each other              | NO           |

More features will come, as they will be implemented. I'll try my best listing them all here.

## Controls

- WASD for moving
- Tab for toggling mouse access
- ...

## Building

All compilers should be supported, but no guarantee is given on MSVC. I only test every build with Clang, so G++ support isn't guaranteed either.

All libraries are included within the repository. Clone with:

```sh
git clone --recursive https://github.com/qaxl/craft
```

Building with CMake (platform-agnostic):

```sh
# Create build directory, you can call this whatever.
mkdir build && cd build
# Generate project files and build the program.
cmake .. && cmake --build .
```

## Platforms Supported

Currently supported platforms are the following:

| Platform  | Support                        |
| --------- | -------                        |
| Android   | Planned                        |
| Linux     | Planned                        |
| Windows   | Supported                      |

## Third Party Libraries Used

- Vulkan for rendering graphics
- Volk for loading the Vulkan library
- VulkanMemoryAllocator for allocating memory in Vulkan
- ImGui for intermediate GUI
- GLM for mathematics (will be replaced soon:tm:)
- SDL for windowing and platform-specific APIs
- stb_image for image loading
- Steamworks for Steam integration (TODO!)

## Attributions

- [vulkan-tutorial.com](https://vulkan-tutorial.com/) for teaching the basics
- [vulkan.org/learn](https://www.vulkan.org/learn) for diving deeper; I recommend this tutorial, if you want to get started!
- [vkguide.dev](https://vkguide.dev/) for dynamic rendering implementation
- ...
