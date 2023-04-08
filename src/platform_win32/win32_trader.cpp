#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#define UNICODE
#include <windows.h>

#pragma comment(lib, "Kernel32.lib")

#include "..\trader.h"

int CALL_CONVENTION
WinMain(HINSTANCE instance,
        HINSTANCE previous_instance,
        LPSTR     command_line,
        int       show_command)
{
  unused(instance);
  unused(previous_instance);
  unused(command_line);
  unused(show_command);

  wchar_t _exe_file_name[MAX_PATH] = {};
  GetModuleFileNameW(NULL, _exe_file_name, array_count(_exe_file_name));

  String_utf16 exe_file_name = string_from_c_string(utf16, _exe_file_name, array_count(_exe_file_name));
  unused(exe_file_name);

  return(0);
}
