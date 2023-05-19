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
       meta_log((utf8 *) "%s(%s): %s check failed\n",             \
                __FUNCTION__, stringify(__LINE__), stringify(c)); \
     }                   \
     else                \
     {                   \
       meta_log((utf8 *) "%s(%s): %s check failed - %s\n",           \
                __FUNCTION__, stringify(__LINE__), stringify(c), m); \
     }                   \
   })                    \

#define expect_message_always(m) expect_break(m)

#if !SHIP_MODE
#define expect(c)            expect_always(c, "")
#define expect_message(c, m) expect_always(c, m)
#define expect_once(c)                                      \
  local_persist b32 concat(__once__, __LINE__) = false;     \
  if (!concat(__once__, __LINE__) && !(c))                  \
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

  u64    high_precision_timer_frequency;
  u64    last_time_stamp;
  u64    last_high_precision_time;
};

global Meta_Info meta_info = {};
#endif

internal void meta_init(void);
internal void meta_log(utf8 *format, ...);

internal void meta_collate_timing_records(void);

#define TRADER_META_H
#endif
