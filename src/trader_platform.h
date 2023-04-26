#ifndef TRADER_PLATFORM_H

struct Global_Platform_State;

extern Global_Platform_State *get_global_platform_state();
extern void platform_print(const char *format, ...);

#define TRADER_PLATFORM_H
#endif
