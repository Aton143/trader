#include "../trader_platform.h"
#include "../trader_ui.h"

struct UI_Context;
struct Global_Platform_State
{
  UI_Context ui_context;
  f32        dt;
};

Global_Platform_State linux_platform_state;

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

internal inline u64 platform_get_processor_time_stamp(void)
{
  u64 ts = 0;
  return(ts);
}

internal inline u64 platform_get_high_precision_timer(void)
{
  u64 ts = 0;
  return(ts);
}

internal void meta_log(utf8 *format, ...)
{
  unused(format);
}

internal Global_Platform_State *platform_get_global_state(void)
{
  return(&linux_platform_state);
}

internal u8 *platform_allocate_memory_pages(u64 bytes, void *start)
{
  unused(bytes);
  unused(start);
  return(NULL);
}
