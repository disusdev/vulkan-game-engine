
#define DEBUG

#define PLATFORM_WIN 1

#if PLATFORM_WIN
#define NOMINMAX
#endif

#if defined(DEBUG)
#define CONSOLE_APP 1
#else
#define CONSOLE_APP 0
#endif

#define VK_USE_PLATFORM_WIN32_KHR