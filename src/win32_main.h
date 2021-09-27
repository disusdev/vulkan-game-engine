
#include <windows.h>
#include <windowsx.h>

typedef double f64;

struct
stWindow
{
  glm::vec2 Size = { 0, 0 };
  void* Instance = nullptr;
  void* WindowHendle = nullptr;
  bool Focused = false;
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
#include "camera.h"
#include "sun.h"
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
  case WM_SETFOCUS:
  {
    g_Engine.Window.Focused = true;
  } break;
  case WM_KILLFOCUS:
  {
    g_Engine.Window.Focused = false;
  } break;
  case WM_MOUSEMOVE:
  {
    //g_Engine.Input.CorrentMousePosition.x = (float)GET_X_LPARAM(LParam);
    //g_Engine.Input.CorrentMousePosition.y = (float)GET_Y_LPARAM(LParam);
  } break;
  //case WM_MOUSELEAVE:
  //{
  //  g_Engine.Input.MousePosition.x = 0.0f;
  //  g_Engine.Input.MousePosition.y = 0.0f;
  //} break;
  case WM_MOVE:
  {
    RECT r;
    GetWindowRect((HWND)g_Engine.Window.WindowHendle, &r);
    g_Engine.Window.Size.x = (float)(r.right - r.left);
    g_Engine.Window.Size.y = (float)(r.bottom - r.top);
    g_Engine.Input.CenterPosition = { r.left + g_Engine.Window.Size.x / 2, r.top + g_Engine.Window.Size.y / 2 };
  } break;
  case WM_SIZE:
  {
    g_Engine.Resize();
    RECT r;
    GetWindowRect((HWND)g_Engine.Window.WindowHendle, &r);
    g_Engine.Window.Size.x = (float)(r.right - r.left);
    g_Engine.Window.Size.y = (float)(r.bottom - r.top);
    g_Engine.Input.CenterPosition = { r.left + g_Engine.Window.Size.x / 2, r.top + g_Engine.Window.Size.y / 2 };
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
  // double nanoseconds_per_count = 1.0e9 / static_cast<double>(frequency.QuadPart);
  return time.QuadPart / static_cast<double>(frequency.QuadPart); // seconds
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
    RECT r;
    GetWindowRect((HWND)g_Engine.Window.WindowHendle, &r);
    g_Engine.Window.Size.x = (float)(r.right - r.left);
    g_Engine.Window.Size.y = (float)(r.bottom - r.top);
    g_Engine.Input.CenterPosition = { r.left + g_Engine.Window.Size.x / 2, r.top + g_Engine.Window.Size.y / 2 };
    SetCursorPos( (int)g_Engine.Input.CenterPosition.x, (int)g_Engine.Input.CenterPosition.y );
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
  g_Engine.Input.RotationDelta.x = 0.0f;
  g_Engine.Input.RotationDelta.y = 0.0f;

  if (g_Engine.Input.GetKeyDown(KEY_TAB))
  {
      SetCursorPos( (int)g_Engine.Input.CenterPosition.x, (int)g_Engine.Input.CenterPosition.y );
      ShowCursor(g_Engine.Renderer.Camera->Locked);
      g_Engine.Renderer.Camera->Locked = !g_Engine.Renderer.Camera->Locked;
  }

  if (g_Engine.Input.WorkInBackground || g_Engine.Window.Focused)
  {
      g_Engine.Input.KeysHold = GetAsyncKeyState('W') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_FORWARD
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('S') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_BACKWARD
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('A') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_LEFT
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('D') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_RIGHT
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('Q') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_Q
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('E') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_E
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState(VK_SPACE) & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_UP
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState(VK_LCONTROL) & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_DOWN
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState(VK_TAB) & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_TAB
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState(VK_LSHIFT) & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_SPEED
        : g_Engine.Input.KeysHold;
      g_Engine.Input.KeysHold = GetAsyncKeyState('L') & 0x8000
        ? g_Engine.Input.KeysHold | enKeyAction::KEY_LOAD
        : g_Engine.Input.KeysHold;

      if (g_Engine.Renderer.Camera->Locked)
      {
        POINT p;
        GetCursorPos(&p);
        g_Engine.Input.CurrentMousePosition = {p.x,p.y};
        g_Engine.Input.RotationDelta = g_Engine.Input.CurrentMousePosition - g_Engine.Input.CenterPosition;
        SetCursorPos( (int)g_Engine.Input.CenterPosition.x, (int)g_Engine.Input.CenterPosition.y );
      }
  }

  g_Engine.Input.KeysDown = (g_Engine.Input.KeysHold
    ^ g_Engine.Input.KeysPrevHold) & g_Engine.Input.KeysHold;
  g_Engine.Input.KeysUp = (g_Engine.Input.KeysHold
    ^ g_Engine.Input.KeysPrevHold) & g_Engine.Input.KeysPrevHold;
  g_Engine.Input.KeysPrevHold = g_Engine.Input.KeysHold;
}

VkSurfaceKHR
CreateSurface(
  VkInstance instance,
  const stWindow& window,
  stDeletionQueue* deletionQueue)
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

  if (deletionQueue)
  deletionQueue->PushFunction([=]{
    vkDestroySurfaceKHR(instance, surface, nullptr);
  });

  return surface;
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

    actualExtent.width = utils::Clip(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = utils::Clip(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}