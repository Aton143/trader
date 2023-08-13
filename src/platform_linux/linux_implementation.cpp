#include <stdio.h>

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#include <sys/mman.h>

#include "../trader_platform.h"
#include "../trader_ui.h"
#include "../trader_render.h"
#include "../trader_network.h"

struct Cursor_Handle
{
  Cursor _handle;
};
Cursor_Handle cursors[cursor_kind_count];

struct Render_Context
{
  Common_Render_Context  common_context;
};

struct UI_Context;
struct Global_Platform_State
{
  Arena           global_arena;
  UI_Context      ui_context;
  Render_Context  render_context;
  f32             dt;

  Display        *display;
  Window          window_handle;

  Atom            atom_TARGETS;
  Atom            atom_CLIPBOARD;
  Atom            atom_UTF8_STRING;
  Atom            atom__NET_WM_STATE;
  Atom            atom__NET_WM_STATE_MAXIMIZED_HORZ;
  Atom            atom__NET_WM_STATE_MAXIMIZED_VERT;
  Atom            atom__NET_WM_STATE_FULLSCREEN;
  Atom            atom__NET_WM_PING;
  Atom            atom__NET_WM_WINDOW_TYPE;
  Atom            atom__NET_WM_WINDOW_TYPE_NORMAL;
  Atom            atom__NET_WM_PID;
  Atom            atom_WM_DELETE_WINDOW;

  b32             has_xfixes;
  i32             xfixes_selection_event;

  XIM             x11_input_method;
  XIC             x11_input_context;

  i32             xkb_event;
  XkbDescPtr      xkb;
};

global Global_Platform_State linux_platform_state;
global_const String_Const_utf8 default_font_path = 
    string_literal_init_type("../assets/ubuntu_default_font/Ubuntu-Regular.ttf", utf8);
global_const f32 default_font_heights[] = {24.0f};

internal void platform_debug_printf(char *format, ...)
{
  char buffer[512] = {};
  va_list args;
  va_start(args, format);

  stbsp_vsnprintf(buffer, array_count(buffer), format, args);
  platform_debug_print(buffer);

  va_end(args);
}

internal b32 platform_open_file(utf8   *file_name,
                                u64     file_name_length,
                                Handle *out_handle)
{
  expect(file_name  != NULL);
  expect(out_handle != NULL);

  if (file_name_length == 0)
  {
    file_name_length = c_string_length(file_name);
  }

  b32 result = true;

  i32 file_descriptor = open((char *) file_name, O_RDWR);
  if (file_descriptor > -1)
  {
    out_handle->generation++;
    out_handle->kind                 = Handle_Kind_File;
    out_handle->file_handle.__handle = file_descriptor;
    out_handle->id                   = scu8(file_name, file_name_length); 
  }
  else
  {
    platform_debug_print_system_error();
  }

  return(result);
}

internal b32 platform_close_file(Handle *handle)
{
  expect(handle != NULL);

  b32 result = false;

  if (!is_nil(handle) && (handle->file_handle.__handle > 0))
  {
    i32 close_result = close(handle->file_handle.__handle);
    if (close_result > -1)
    {
      make_nil(handle);
      result = true;
    }
    else
    {
      platform_debug_print_system_error();
    }
  }

  return(result);
}

internal b32 platform_open_file_for_appending(utf8   *file_path,
                                              u64     file_path_size,
                                              Handle *out_handle)
{
  b32 result = platform_open_file(file_path, file_path_size, out_handle);

  if (result)
  {
    if (fcntl(F_SETFD, O_RDONLY) < 0)
    {
      platform_debug_print_system_error();
      result = false;
    }

    if (result && fcntl(F_SETFL, O_APPEND) < 0)
    {
      platform_debug_print_system_error();
      result = false;
    }
  }

  return(result);
}

internal File_Buffer platform_read_entire_file(Arena *arena, Handle *handle)
{
  expect(handle != NULL);
  expect(arena  != NULL);

  File_Buffer result = {};

  struct stat64 file_data;
  if (fstat64(handle->file_handle.__handle, &file_data) >= 0)
  {
    i64  file_size = (i64) file_data.st_size;
    u8  *to_put    = (u8 *) arena_push(arena, file_size);

    i64 bytes_left = file_size;
    i64 bytes_read;

    while (bytes_left > 0)
    {
      bytes_read  = (i64) read(handle->file_handle.__handle, to_put, bytes_left);
      bytes_left -= bytes_read;

      if (bytes_read < 0)
      {
        platform_debug_print_system_error();
        arena_pop(arena, file_size);
        break;
      }
    }

    if (bytes_left == 0)
    {
      result.data = to_put;
      result.size = file_size;
      result.used = file_size;

      handle->generation++;
    }
  }
  else
  {
    platform_debug_print_system_error();
  }

  return(result);
}

internal File_Buffer platform_open_and_read_entire_file(Arena *arena,
                                                        utf8  *file_path,
                                                        u64    file_path_size)
{
  File_Buffer result = {};

  Handle file_handle = {};
  if (platform_open_file(file_path, file_path_size, &file_handle))
  {
    result = platform_read_entire_file(arena, &file_handle);
    close(file_handle.file_handle.__handle);
  }

  return(result);
}

internal inline u64 platform_get_high_precision_time(void)
{
  timespec _ts;
  u64 ts;

  clock_gettime(CLOCK_MONOTONIC, &_ts);
  ts = (_ts.tv_sec * 1e6) + _ts.tv_nsec;

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
  fputs(message, stderr);
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

internal void platform_thread_init(void)
{
  for (u32 thread_context_index = 0;
       thread_context_index < thread_count;
       ++thread_context_index)
  {
    thread_contexts[thread_context_index].local_temp_arena.arena = arena_alloc(global_temp_arena_size, 1, NULL);
  }
}

internal void meta_init(void)
{
  timespec _freq;
  if (clock_getres(CLOCK_MONOTONIC, &_freq) == -1)
  {
    platform_debug_print_system_error();
  }

  meta_info.high_precision_timer_frequency = (_freq.tv_sec * 1e6) + _freq.tv_nsec;

  Arena *temp_arena = get_temp_arena();
  set_temp_arena_wait(1);

  temp_arena->used = copy_string_lit(temp_arena->start, (utf8 *) "./logs/");
  temp_arena->used -= 1;

  time_t _now = time(NULL);
  tm now = *localtime(&_now);

  temp_arena->used = stbsp_snprintf((char *) (temp_arena->start + temp_arena->used),
                                    (int) (temp_arena->size - temp_arena->used - 1),
                                    "%04d_%02d_%02d_%02d_%02d_%02d",
                                    now.tm_year + 1900,
                                    now.tm_mon + 1,
                                    now.tm_mday,
                                    now.tm_hour,
                                    now.tm_min,
                                    now.tm_sec);
  temp_arena->used -= 1;

  temp_arena->used += copy_string_lit(&temp_arena->start[temp_arena->used], (utf8 *) ".log");
  temp_arena->used -= 1;
}



internal Key_Event platform_convert_key_to_our_key(u64 key_value)
{
    switch (key_value)
    {
    case XK_Tab:          return key_event_tab;
    case XK_Escape:       return key_event_escape;
    case XK_Pause:        return key_event_pause;
    case XK_Up:           return key_event_up_arrow;
    case XK_Down:         return key_event_down_arrow;
    case XK_Left:         return key_event_left_arrow;
    case XK_Right:        return key_event_right_arrow;
    case XK_BackSpace:    return key_event_backspace;
    case XK_Return:       return key_event_enter;
    case XK_Delete:       return key_event_delete;
    case XK_Insert:       return key_event_insert;
    case XK_space:        return key_event_space;
    case XK_Home:         return key_event_home;
    case XK_End:          return key_event_end;
    case XK_Page_Up:      return key_event_page_up;
    case XK_Page_Down:    return key_event_page_down;
    case XK_Caps_Lock:    return key_event_caps_lock;
    case XK_Num_Lock:     return key_event_num_lock;
    case XK_Scroll_Lock:  return key_event_scroll_lock;
    case XK_Menu:         return key_event_menu;
    case XK_Shift_L:      return key_event_left_shift;
    case XK_Shift_R:      return key_event_right_shift;
    case XK_Control_L:    return key_event_left_ctrl;
    case XK_Control_R:    return key_event_right_ctrl;
    case XK_Alt_L:        return key_event_left_alt;
    case XK_Alt_R:        return key_event_right_alt;
    case XK_Super_L:      return key_mod_event_super;
    case XK_Super_R:      return key_mod_event_super;
    case XK_F1:           return key_event_f1;
    case XK_F2:           return key_event_f2;
    case XK_F3:           return key_event_f3;
    case XK_F4:           return key_event_f4;
    case XK_F5:           return key_event_f5;
    case XK_F6:           return key_event_f6;
    case XK_F7:           return key_event_f7;
    case XK_F8:           return key_event_f8;
    case XK_F9:           return key_event_f9;
    case XK_F10:          return key_event_f10;
    case XK_F11:          return key_event_f11;
    case XK_F12:          return key_event_f12;
    case XK_KP_0:         return key_event_keypad_0;
    case XK_KP_1:         return key_event_keypad_1;
    case XK_KP_2:         return key_event_keypad_2;
    case XK_KP_3:         return key_event_keypad_3;
    case XK_KP_4:         return key_event_keypad_4;
    case XK_KP_5:         return key_event_keypad_5;
    case XK_KP_6:         return key_event_keypad_6;
    case XK_KP_7:         return key_event_keypad_7;
    case XK_KP_8:         return key_event_keypad_8;
    case XK_KP_9:         return key_event_keypad_9;
    case XK_KP_Multiply:  return key_event_keypad_multiply;
    case XK_KP_Add:       return key_event_keypad_add;
    case XK_KP_Subtract:  return key_event_keypad_subtract;
    case XK_KP_Decimal:   return key_event_keypad_decimal;
    case XK_KP_Delete:    return key_event_delete;
    case XK_KP_Divide:    return key_event_keypad_divide;
    case XK_KP_Enter:     return key_event_enter;
    case XK_0:            return key_event_0;
    case XK_1:            return key_event_1;
    case XK_2:            return key_event_2;
    case XK_3:            return key_event_3;
    case XK_4:            return key_event_4;
    case XK_5:            return key_event_5;
    case XK_6:            return key_event_6;
    case XK_7:            return key_event_7;
    case XK_8:            return key_event_8;
    case XK_9:            return key_event_9;
    case XK_a: case XK_A: return key_event_a;
    case XK_b: case XK_B: return key_event_b;
    case XK_c: case XK_C: return key_event_c;
    case XK_d: case XK_D: return key_event_d;
    case XK_e: case XK_E: return key_event_e;
    case XK_f: case XK_F: return key_event_f;
    case XK_g: case XK_G: return key_event_g;
    case XK_h: case XK_H: return key_event_h;
    case XK_i: case XK_I: return key_event_i;
    case XK_j: case XK_J: return key_event_j;
    case XK_k: case XK_K: return key_event_k;
    case XK_l: case XK_L: return key_event_l;
    case XK_m: case XK_M: return key_event_m;
    case XK_n: case XK_N: return key_event_n;
    case XK_o: case XK_O: return key_event_o;
    case XK_p: case XK_P: return key_event_p;
    case XK_q: case XK_Q: return key_event_q;
    case XK_r: case XK_R: return key_event_r;
    case XK_s: case XK_S: return key_event_s;
    case XK_t: case XK_T: return key_event_t;
    case XK_u: case XK_U: return key_event_u;
    case XK_v: case XK_V: return key_event_v;
    case XK_w: case XK_W: return key_event_w;
    case XK_x: case XK_X: return key_event_x;
    case XK_y: case XK_Y: return key_event_y;
    case XK_z: case XK_Z: return key_event_z;
    case XK_grave:        return key_event_grave_accent;
    case XK_minus:        return key_event_minus;
    case XK_equal:        return key_event_equal;
    case XK_bracketleft:  return key_event_left_bracket;
    case XK_bracketright: return key_event_right_bracket;
    case XK_semicolon:    return key_event_semicolon;
    case XK_apostrophe:   return key_event_apostrophe;
    case XK_comma:        return key_event_comma;
    case XK_period:       return key_event_period;
    case XK_slash:        return key_event_slash;
    case XK_backslash:    return key_event_backslash;
    default:              return key_event_none;
    }
 }

internal inline u64 linux_microseconds_from_timespec(timespec ts)
{
  u64 microseconds = (ts.tv_nsec / thousand(1)) + (million(1) * ts.tv_sec);
  return(microseconds);
}

internal inline u64 platform_get_time_in_microseconds(void)
{
  u64 microseconds;

  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  microseconds = linux_microseconds_from_timespec(time);
  return(microseconds);
}

internal f64 platform_get_time_in_seconds(void)
{
  f64 seconds;

  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  seconds = (time.tv_nsec / ((f64) 1e9)) + ((f64) time.tv_sec);
  return(seconds);
}

internal void render_debug_print_compile_errors(void *data)
{
  i32 *gl_shader = (i32 *) data;

  utf8 *info_log = (utf8 *) stack_alloc(512);
  zero_memory_block(info_log, 512);

  i32  success;
  glGetShaderiv(*gl_shader, GL_COMPILE_STATUS, &success);

  if (!success)
  {
    glGetShaderInfoLog(*gl_shader, sizeof(info_log), NULL, (GLchar *) info_log);
    meta_log(info_log);
  }
}
