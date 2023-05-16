#ifndef TRADER_ASSERT_H

#if OS_WINDOWS
# define debug_break() __debugbreak()
#endif

#define assert_break(m) debug_break()
#define assert_always(c) macro_do( if (!(c)) { assert_break(c); } )
#define assert_message_always(m) assert_break(m)

#if !SHIP_MODE
#define assert(c)         assert_always(c)
#define assert_message(m) assert_message_always(m)
#define assert_once(c)    local_persist b32 __once__ = false; if (!__once__ && !(c)) (__once__ = true, assert_break(m))
#else
#define assert(c)
#define assert_message(m)
#define assert_once(c)
#endif

#define TRADER_ASSERT_H
#endif
