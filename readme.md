

# **Simple Game Engine in C++ [WIP]**

> All work is in progress and plans can be changed over time.

## Spec:

- Platform: Windows x64
- Rendering: Vulkan

## External:

- glm (https://github.com/g-truc/glm)
- tinyobjloader (https://github.com/tinyobjloader/tinyobjloader)
- vma (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

## Planed Features:

- [ ] Window Management
- [ ] Input Handling
- [ ] File System
- [ ] Graphics System
- [ ] Scene Managment
- [ ] Collision System
- [ ] Multithreading
- [ ] Asset Management
- [ ] Animation
- [ ] Audio
- [ ] Event System
- [ ] Networking

> *“First, solve the problem. Then, write the code.”* – John Johnson

## Plan:

1. Create Window (Win32)
2. Game Loop
3. Input System (Win32 Keyboard)
4. Initialize:
    1. Vulkan init
    2. Commands
    3. Renderpass
5. Draw triangle:
    1. Shaders
    2. Render pipeline
    3. Draw triangle
6. File system
7. Draw mesh:
    1. Vertex Buffer
    2. Push Constants
    3. Load OBJ
    4. Depth Buffer
8. Buffers:
    1. Double Buffers
    2. Decriptor Sets
    3. Storage Buffers
9. Textures:
    1. Memory Transfer
    2. Vulkan Images
    3. Loading Images
    4. Drawing Images
10. GPU Driven Rendering

## Actual names

0. [Simple Game Engine in C++] How to setup project on Windows with premake C++
1. [Simple Game Engine in C++] How to create window on Windows C++
2. [Simple Game Engine in C++] How to create Game Loop C++
3. [Simple Game Engine in C++] How to create Input System C++ : Part 1 [Windows]
4. [Simple Game Engine in C++] How to create Input System C++ : Part 2 [Windows/Controller]
5. [Simple Game Engine in C++] How to create Vulkan Renderer C++ : Part 1 [Initialize]
6. [Simple Game Engine in C++] How to create Vulkan Renderer C++ : Part 2 [Draw triangle]
7. [Simple Game Engine in C++] How to create File System C++