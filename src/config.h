
#define DEBUG

#define PLATFORM_WIN 1

#if PLATFORM_WIN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#if defined(DEBUG)
#define CONSOLE_APP 1
#else
#define CONSOLE_APP 0
#endif

#define VK_USE_PLATFORM_WIN32_KHR

#define MAX_ENTITY_MESH_COUNT 2048
#define MAX_TEXTURE_COUNT 2048
#define MAX_MESH_COUNT 2048


// #define MAX_ENTITIES_COUNT 1024
//#define MAX_RENDER_OBJECT_COUNT 1024