

# **Vulkan Game Engine**

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
3. Input System (Keyboard)
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