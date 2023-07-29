#ifndef TRADER_PLATFORM_H

struct Handle;
struct Cursor_Handle;
typedef u32 Cursor_Kind;

enum
{
  cursor_kind_none = 0,

  cursor_kind_pointer,
  cursor_kind_finger_pointer,
  cursor_kind_grab,

  cursor_kind_left_right_direction,
  cursor_kind_up_down_direction,
  cursor_kind_up_down_left_right_direction,

  cursor_kind_text_selection,

  cursor_kind_count,
};

extern Cursor_Handle cursors[];

typedef u16 Key_Event;

typedef u32 (THREAD_CALL_CONVENTION *Thread_Routine)(void *arg);
struct Thread_Handle;

internal Global_Platform_State *platform_get_global_state(void);
internal Arena *platform_get_global_arena(void);

unimplemented internal void  *platform_allocate_memory_pages(u64 bytes);

internal void platform_debug_print(char *text);
internal void platform_debug_printf(char *format, ...);
internal void platform_debug_print_system_error();

internal void platform_initialize(void);

internal File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size);

internal b32         platform_open_file(utf8 *file_name, u64 file_name_length, Handle *out_handle);
internal File_Buffer platform_read_entire_file(Handle *handle);

internal b32 platform_open_file_for_appending(utf8 *file_name, u64 file_name_length, Handle *out_handle);
internal b32 platform_append_to_file(Handle *handle, utf8 *format, va_list args);

internal void platform_push_notify_dir(utf8 *dir_path, u64 dir_path_length);
internal void platform_pop_notify_dir(void);

internal void platform_start_collect_notifications(void);
internal void platform_collect_notifications(void);

internal b32 platform_did_file_change(utf8 *file_name, u64 file_name_length);
internal String_Const_utf8 platform_get_file_name_from_path(String_Const_utf8 *path);

internal u64 platform_get_high_precision_timer(void);
internal u64 platform_get_processor_time_stamp(void);

internal double platform_convert_high_precision_time_to_seconds(u64 high_precision_time);

internal Key_Event platform_convert_key_to_our_key(u64 key_value);

internal String_Const_utf8 platform_get_file_from_system_prompt();
internal File_Buffer       platform_open_and_read_entire_file_from_system_prompt(Arena *arena);

internal void  platform_set_cursor(Cursor_Kind cursor);

internal String_Const_utf8 platform_read_clipboard_contents(Arena *arena);
internal void              plaform_write_clipboard_contents(String_utf8 string);

// NOTE(antonio): threads
internal Thread_Handle platform_create_thread(Thread_Routine routine, void *routine_arg);

#define TRADER_PLATFORM_H
#endif
