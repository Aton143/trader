#ifndef TRADER_PLATFORM_H

internal Global_Platform_State *get_global_platform_state();

internal void platform_print(const char *format, ...);
internal void platform_initialize();

internal File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size);
internal b32         platform_open_file(utf8 *file_name, u64 file_name_length, Handle *out_handle);
internal File_Buffer platform_read_entire_file(Handle *handle);

internal void platform_push_notify_dir(utf8 *dir_path, u64 dir_path_length);
internal void platform_pop_notify_dir(void);

internal void platform_start_collect_notifications(void);
internal void platform_collect_notifications(void);

internal b32 platform_did_file_change(utf8 *file_name, u64 file_name_length);
internal String_Const_utf8 platform_get_file_name_from_path(String_Const_utf8 *path);

#define TRADER_PLATFORM_H
#endif
