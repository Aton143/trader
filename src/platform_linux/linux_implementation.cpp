#include "../trader_platform.h"
internal void platform_debug_printf(char *format, ...)
{
  char buffer[512] = {};
  va_list args;
  va_start(args, format);

  stbsp_vsnprintf(buffer, array_count(buffer), format, args);
  platform_debug_print(buffer);

  va_end(args);
}

internal b32 platform_open_file(utf8 *file_name, u64 file_name_length, Handle *out_handle)
{
  b32 result = false;

  unused(file_name);
  unused(file_name_length);
  unused(out_handle);

  return(result);
}
