# vulkan-playground

Vulkan testing code as I follow and play around vulkan-tutorial.com.

The project is written in C++17 and includes CMake rules and a Resource Compiler utility to automatically compile shaders into [SPIR-V](https://en.wikipedia.org/wiki/Standard_Portable_Intermediate_Representation) byte-code using [Google's `glslc`]([https://github.com/google/shaderc/tree/master/glslc](https://github.com/google/shaderc/tree/master/glslc)) and embed the resources in the executable.

Tested with:
* Windows 10: Visual Studio 2019, Vulkan SDK 1.1.121.2
* Ubuntu 19.10: clang 9.0, Vulkan SDK 1.1.126 from the [Ubuntu Repo](https://packages.lunarg.com/)

Requires Vulkan-Hpp repo cloned at the same level:

`git clone --recurse-submodules https://github.com/KhronosGroup/Vulkan-Hpp.git`
