
#include <windows.h>

static bool g_IsRun = false;

LRESULT CALLBACK
WindowProc(
  HWND   Window,
  UINT   Message,
  WPARAM WParam,
  LPARAM LParam
)
{
  switch (Message)
  {
  case WM_DESTROY:
  case WM_CLOSE:
  {
    g_IsRun = false;
  } break;
  }

  return DefWindowProc(Window, Message, WParam, LParam);
}

int CALLBACK
WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR CmdLine,
  int ShowCode)
{
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = &WindowProc;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = L"VulkanEngineWindowClass";

  RegisterClass(&WindowClass);

  HWND WindowHendle = CreateWindowEx(
    0,
    WindowClass.lpszClassName,
    L"Vulkan Engine",
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, //WS_OVERLAPPEDWINDOW,//|WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    0,
    0,
    Instance,
    0
  );

  if (WindowHendle)
  {
    g_IsRun = true;

    ShowWindow(WindowHendle, SW_SHOW);

    while (g_IsRun)
    {
      MSG Message;
      while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }

      if (!g_IsRun)
      {
        break;
      }

      HDC DeviceContext = GetDC(WindowHendle);

      // SWAP
      SwapBuffers(DeviceContext);
      ReleaseDC(WindowHendle, DeviceContext);
    }
  }

  return (0);
}