# vulkan-playground

Vulkan testing code as I follow and play around vulkan-tutorial.com.

The project is written in C++17 and includes CMake rules and a Resource Compiler utility to automatically compile shaders into [SPIR-V](https://en.wikipedia.org/wiki/Standard_Portable_Intermediate_Representation) byte-code using [Google's `glslc`]([https://github.com/google/shaderc/tree/master/glslc](https://github.com/google/shaderc/tree/master/glslc)) and embed the resources in the executable.

Tested with Vulkan SDK 1.1.121.2 using Visual Studio 2019.
