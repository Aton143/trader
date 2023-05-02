#ifndef TRADER_PLATFORM_H

extern Global_Platform_State *get_global_platform_state();

extern void platform_print(const char *format, ...);
extern void platform_initialize();

#define TRADER_PLATFORM_H
#endif
