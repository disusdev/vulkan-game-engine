
#include <windows.h>

#include <assert.h>

#include <deque>
#include <functional>
#include <optional>
#include <set>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "meshoptimizer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef double f64;

struct
stWindow
{
  void* Instance = nullptr;
  void* WindowHendle = nullptr;
};

// ############################################################################
// # sys
// ############################################################################

namespace sys
{

LRESULT CALLBACK
WindowProc(
  HWND   Window,
  UINT   Message,
  WPARAM WParam,
  LPARAM LParam
);

f64
GetTime();

bool
WindowCreate(
  const wchar_t* WindowName,
  stWindow* Window
);

void
SwapBuffers(
  stWindow& Window
);

void
GetMessages();

void
UpdateInput();

}

// ############################################################################
// # Win32 main
// ############################################################################

#include "input.h"
#include "engine.h"

engine g_Engine = {};

int CALLBACK
WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR CmdLine,
  int ShowCode)
{
  g_Engine.Window.Instance = Instance;

  return g_Engine.Run();
}

// ############################################################################
// # sys functions
// ############################################################################

LRESULT CALLBACK
sys::WindowProc(
  HWND   Window,
  UINT   Message,
  WPARAM WParam,
  LPARAM LParam
)
{
  switch (Message)
  {
  case WM_SIZE:
  {
    g_Engine.Resize();
  } break;
  case WM_DESTROY:
  case WM_CLOSE:
  {
    g_Engine.IsRunning = false;
  } break;
  }

  return DefWindowProc(Window, Message, WParam, LParam);
}

f64
sys::GetTime()
{
  LARGE_INTEGER time;
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&time);
  double nanoseconds_per_count = 1.0e9 / static_cast<double>(frequency.QuadPart);
  return time.QuadPart * nanoseconds_per_count;
}

bool
sys::WindowCreate(
  const wchar_t* WindowName,
  stWindow* Window
)
{
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = &WindowProc;
  WindowClass.hInstance = (HINSTANCE)Window->Instance;
  WindowClass.lpszClassName = L"EngineWindowClass";

  RegisterClass(&WindowClass);

  Window->WindowHendle = CreateWindowEx(
    0,
    WindowClass.lpszClassName,
    WindowName,
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
    // | WS_OVERLAPPEDWINDOW, | WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    1280,
    720,
    0,
    0,
    (HINSTANCE)Window->Instance, //GetModuleHandle(NULL),
    0
  );

  if (Window->WindowHendle)
  {
    ShowWindow((HWND)Window->WindowHendle, SW_SHOW);
    return true;
  }

  return false;
}

void
sys::SwapBuffers(
  stWindow& Window
)
{
  HDC DeviceContext = GetDC((HWND)Window.WindowHendle);
  // SWAP
  SwapBuffers(DeviceContext);
  ReleaseDC((HWND)Window.WindowHendle, DeviceContext);
}

void
sys::GetMessages()
{
  MSG Message;
  while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&Message);
    DispatchMessage(&Message);
  }
}

void
sys::UpdateInput()
{
  g_Engine.Input.KeysHold = 0x0;

  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_UP) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_FORWARD
    : g_Engine.Input.KeysHold;
  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_DOWN) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_BACKWARD
    : g_Engine.Input.KeysHold;
  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_LEFT) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_LEFT
    : g_Engine.Input.KeysHold;
  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_RIGHT) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_RIGHT
    : g_Engine.Input.KeysHold;
  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_SPACE) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_UP
    : g_Engine.Input.KeysHold;
  g_Engine.Input.KeysHold = GetAsyncKeyState(VK_CONTROL) & 0x8000
    ? g_Engine.Input.KeysHold | enKeyAction::KEY_UP
    : g_Engine.Input.KeysHold;

  g_Engine.Input.KeysDown = (g_Engine.Input.KeysHold
    ^ g_Engine.Input.KeysPrevHold) & g_Engine.Input.KeysHold;
  g_Engine.Input.KeysUp = (g_Engine.Input.KeysHold
    ^ g_Engine.Input.KeysPrevHold) & g_Engine.Input.KeysPrevHold;
  g_Engine.Input.KeysPrevHold = g_Engine.Input.KeysHold;
}

VkSurfaceKHR
CreateSurface(
  VkInstance instance,
  const stWindow& window)
{
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  {
    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
    SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    SurfaceCreateInfo.pNext = nullptr;
    SurfaceCreateInfo.flags = 0;
    SurfaceCreateInfo.hinstance = (HINSTANCE) window.Instance;
    SurfaceCreateInfo.hwnd = (HWND) window.WindowHendle;
    VkResult Result = vkCreateWin32SurfaceKHR(
      instance, &SurfaceCreateInfo, nullptr, &surface
    );
    assert(Result == VK_SUCCESS);
  }
  return surface;
}

template<class T> 
const T& max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template<class T> 
const T& min(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return max(lower, min(n, upper));
}

VkExtent2D
ChooseSwapExtent(
  const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  else
  {
    VkExtent2D actualExtent = { 1264, 681 };

    actualExtent.width = clip(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = clip(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}