#ifndef TRADER_PLATFORM_H

extern Global_Platform_State *get_global_platform_state();

extern void platform_print(const char *format, ...);
extern void platform_initialize();
extern File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size);

extern void platform_push_notify_dir(utf8 *dir_path, u64 dir_path_length);
void platform_collect_notifications(void);
extern void platform_pop_notify_dir(void);

extern b32 platform_did_file_change(utf8 *file_name, u64 file_name_length);

#define TRADER_PLATFORM_H
#endif
