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
global Key_Event             keycode_lookup_table_physical[256];
global Key_Event             keycode_lookup_table_language[256];

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

internal b32 platform_open_file_for_appending(utf8 *file_path, u64 file_path_size, Handle *out_handle)
{
  unused(file_path);
  unused(file_path_size);
  unused(out_handle);
  return(false);
}

internal inline u64 platform_get_high_precision_timer(void)
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
    case XK_Tab:         return key_event_tab;
    case XK_Escape:      return key_event_escape;
    case XK_Pause:       return key_event_pause;
    case XK_Up:          return key_event_up_arrow;
    case XK_Down:        return key_event_down_arrow;
    case XK_Left:        return key_event_left_arrow;
    case XK_Right:       return key_event_right_arrow;
    case XK_BackSpace:   return key_event_backspace;
    case XK_Return:      return key_event_enter;
    case XK_Delete:      return key_event_delete;
    case XK_Insert:      return key_event_insert;
    case XK_space:       return key_event_space;
    case XK_Home:        return key_event_home;
    case XK_End:         return key_event_end;
    case XK_Page_Up:     return key_event_page_up;
    case XK_Page_Down:   return key_event_page_down;
    case XK_Caps_Lock:   return key_event_caps_lock;
    case XK_Num_Lock:    return key_event_num_lock;
    case XK_Scroll_Lock: return key_event_scroll_lock;
    case XK_Menu:        return key_event_menu;
    case XK_Shift_L:     return key_event_left_shift;
    case XK_Shift_R:     return key_event_right_shift;
    case XK_Control_L:   return key_event_left_ctrl;
    case XK_Control_R:   return key_event_right_ctrl;
    case XK_Alt_L:       return key_event_left_alt;
    case XK_Alt_R:       return key_event_right_alt;
    case XK_Super_L:     return key_mod_event_super;
    case XK_Super_R:     return key_mod_event_super;
                         /*
                            case                      return key_event_num_lock;
                            case                      return key_event_print_screen;
                            case                      return key_event_pause;
                            case                      return key_event_keypad_0;
                            case                      return key_event_keypad_1;
                            case                      return key_event_keypad_2;
                            case                      return key_event_keypad_3;
                            case                      return key_event_keypad_4;
                            case                      return key_event_keypad_5;
                            case                      return key_event_keypad_6;
                            case                      return key_event_keypad_7;
                            case                      return key_event_keypad_8;
                            case                      return key_event_keypad_9;
                            case                      return key_event_keypad_decimal;
                            case                      return key_event_keypad_divide;
                            case                      return key_event_keypad_multiply;
                            case                      return key_event_keypad_subtract;
                            case                      return key_event_keypad_add;
                            case                      return key_event_keypad_enter;
                            case                      return key_event_left_shift;
                            case                      return key_event_left_ctrl;
                            case                      return key_event_left_alt;
                            case                      return key_event_left_super;
                            case                      return key_event_right_shift;
                            case                      return key_event_right_ctrl;
                            case                      return key_event_right_alt;
                            case                      return key_event_right_super;
                            case                      return key_event_menu;
                            case                      return key_event_0;
                            case                      return key_event_1;
                            case                      return key_event_2;
                            case                      return key_event_3;
                            case                      return key_event_4;
                            case                      return key_event_5;
                            case                      return key_event_6;
                            case                      return key_event_7;
                            case                      return key_event_8;
                            case                      return key_event_9;
                            case                      return key_event_a;
                            case                      return key_event_b;
                            case                      return key_event_c;
                            case                      return key_event_d;
                            case                      return key_event_e;
                            case                      return key_event_f;
                            case                      return key_event_g;
                            case                      return key_event_h;
                            case                      return key_event_i;
                            case                      return key_event_j;
                            case                      return key_event_k;
                            case                      return key_event_l;
                            case                      return key_event_m;
                            case                      return key_event_n;
                            case                      return key_event_o;
                            case                      return key_event_p;
                            case                      return key_event_q;
                            case                      return key_event_r;
                            case                      return key_event_s;
                            case                      return key_event_t;
                            case                      return key_event_u;
                            case                      return key_event_v;
                            case                      return key_event_w;
                            case                      return key_event_x;
                            case                      return key_event_y;
                            case                      return key_event_z;
                            case                      return key_event_f1;
                            case                      return key_event_f2;
                            case                      return key_event_f3;
                            case                      return key_event_f4;
                            case                      return key_event_f5;
                            case                      return key_event_f6;
                         case                      return key_event_f7;
                         case                      return key_event_f8;
                         case                      return key_event_f9;
                         case                      return key_event_f10;
                         case                      return key_event_f11;
                         case                      return key_event_f12;
                         */
    default:                  return key_event_none;
    }
}
