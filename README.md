# Craft

My own custom-built game engine for my own projects. It's not currently not ready for production use.

> [!NOTE]  
> Craft is still under heavy development. Do not expect a functional product in its current state.

## Features

| Feature                                               | Implemented? |
| ----------------------------------------------------- | ------------ |
| Play as rectangles that shoot each other              | NO           |
| Implement Vulkan abstractions for most things         | NO           |

More features will come, as they will be implemented. I'll try my best listing them all here.

## Optional Features

Using networking is optional via Steamworks SDK. It's not distributed by default in this repository, and can be instead downloaded from: <https://partner.steamgames.com/>.

After downloading you should be able to see the `sdk/` folder inside `steamworks_sdk_xxx`, and you should copy it over to `deps/steamworks/sdk`.

After that you should enable CMake option `BUILD_WITH_STEAMSDK`, such as:

```sh
~ cmake -DBUILD_WITH_STEAMSDK=TRUE <your other parameters here> ..
```

...and you're ready to go.

## Platforms Supported

Currently supported platforms are the following:

| Platform  | Support                        |
| --------- | -------                        |
| Android   | Planned                        |
| Linux     | Planned                        |
| Windows   | Supported                      |
| macOS     | None                           |
| iOS       | None                           |

If platform is supported, it means that the project should compile and run successfully without any extra steps from this repository.

If platform support is planned, it means that I plan on adding the support for that platform in the near future.

If platform support is none, do not expect any support for the platform in near future. Why is this project not supporting Apple devices? Simple as one sentence: I do not own any Apple devices, hence I cannot develop in their environment.
