#include <stdio.h>

#include <errno.h>
#include <sys/mman.h>

#include "../trader_platform.h"
#include "../trader_ui.h"
#include "../trader_render.h"
#include "../trader_network.h"

struct Render_Context
{
  Common_Render_Context common_context;
};

struct UI_Context;
struct Global_Platform_State
{
  Arena          global_arena;
  UI_Context     ui_context;
  Render_Context render_context;
  f32            dt;
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
  u64 mmap_additional_flags = (start != NULL) ? MAP_FIXED : 0;

  i32 backing_fd            = 0;
  i32 offset                = 0;

  u8 *mapped_pages = (u8 *) mmap(start,
                                 bytes,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | mmap_additional_flags,
                                 backing_fd, offset);

  if (mapped_pages == MAP_FAILED)
  {
    mapped_pages = NULL;
    platform_debug_print_system_error();
  }

  return(mapped_pages);
}

internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive_buffer)
{
  Network_Return_Code result = network_error_unimplemented;
  unused(state);
  unused(in_socket);
  unused(receive_buffer);
  return(result);
}

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *send)
{
  Network_Return_Code result = network_error_unimplemented;
  unused(state);
  unused(in_socket);
  unused(send);
  return(result);
}

internal void platform_set_cursor(Cursor_Kind cursor)
{
  unused(cursor);
}

internal String_Const_utf8 platform_read_clipboard_contents(Arena *arena)
{
  String_Const_utf8 contents = {};
  unused(arena);
  return(contents);
}

internal void platform_write_clipboard_contents(String_utf8 string)
{
  unused(string);
}

internal Arena *platform_get_global_arena(void)
{
  Arena *global_arena = &linux_platform_state.global_arena;
  return(global_arena);
}

internal inline Render_Context *render_get_context()
{
  Render_Context *render = &platform_get_global_state()->render_context;
  return(render);
}

internal void platform_debug_print(char *message)
{
  fputs(message, stdout);
}

global_const u8 platform_path_separator = '\\';
global_const u8 unix_path_separator = '/';

internal String_Const_utf8 platform_get_file_name_from_path(String_Const_utf8 *path)
{
  String_Const_utf8 result = {};

  u64 last_separator_pos = 0;
  for (u64 path_char_index = 0;
       path_char_index < path->size;
       ++path_char_index)
  {
    if ((path->str[path_char_index] == platform_path_separator) ||
        (path->str[path_char_index] == unix_path_separator))
    {
      last_separator_pos = path_char_index;
    }
  }

  u64 char_pos = (last_separator_pos == 0) ? last_separator_pos : (last_separator_pos + 1);
  result.str   = &path->str[char_pos];
  result.size  = path->size - char_pos;

  return(result);
}

internal inline f64 platform_convert_high_precision_time_to_seconds(u64 hpt)
{
  f64 seconds = 0.0;
  unused(hpt);
  return(seconds);
}

internal void platform_debug_print_system_error()
{
  perror(NULL);
}
