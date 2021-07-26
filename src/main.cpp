
#include "config.h"

#if CONSOLE_APP
#include "stdio.h"
#endif

#if PLATFORM_WIN

#include "win32_main.h"

#if CONSOLE_APP

int main()
{
  return WinMain(GetModuleHandle(NULL), NULL,NULL, 1);
}

#endif

#endif