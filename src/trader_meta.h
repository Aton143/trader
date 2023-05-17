#ifndef TRADER_META_H
#include "trader_handle.h"

#if OS_WINDOWS
# define debug_break() __debugbreak()
#endif

#define expect_break(m) debug_break()
#define expect_always(c, m) \
  macro_do(              \
   if (!(c))             \
   {                     \
     expect_break(c);    \
     if (*m == '\0')     \
     {                   \
       meta_log((utf8 *) "%s check failed at line %s in function %s\n", \
              stringify(c), stringify(__LINE__), __FUNCTION__);         \
     }                   \
     else                \
     {                   \
       meta_log((utf8 *) "%s check failed at line %s in function %s - %s\n", \
              stringify(c), stringify(__LINE__), __FUNCTION__, m);           \
     }                   \
   })                    \

#define expect_message_always(m) expect_break(m)

#if !SHIP_MODE

#define expect(c)            expect_always(c, "")
#define expect_message(c, m) expect_always(c, m)
#define expect_once(c)                                    \
  local_persist b32 concat(__once__, __LINE__) = false;   \
  if (!concat(__once__, __LINE__) && !(c))                \
    (concat(__once__, __LINE__) = true, expect_break(m))  \

#else
#define expect(c)
#define expect_message(m)
#define expect_once(c)
#endif

#if !SHIP_MODE
struct Meta_Info
{
  Handle log_handle;
};

global Meta_Info meta_info = {};
#endif

internal void meta_init(void);
internal void meta_log(utf8 *format, ...);

#define TRADER_META_H
#endif
