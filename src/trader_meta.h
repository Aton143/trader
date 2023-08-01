#ifndef TRADER_META_H
#include "trader_handle.h"

#if OS_WINDOWS
#elif COMPILER_GCC
# if ARCH_X64
internal inline void __debugbreak()
{
  asm volatile("int3");
}
# endif
#endif

# define debug_break() __debugbreak()

#define expect_break(m) debug_break()
#define expect_always(c, m) \
  macro_do(              \
   if (!(c))             \
   {                     \
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
     expect_break(c);    \
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

#define TIMED_BLOCK() Timed_Block_RAII \
  concat(timed_block_, __LINE__)(__COUNTER__, __FILE__, __FUNCTION__, __LINE__)
#define TIMED_BLOCK_START(...) Timed_Block \
  __timed_block__ = meta_start_timed_block(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define TIMED_BLOCK_END() meta_end_timed_block(&__timed_block__)

struct Timing_Record
{
  utf8 *file_name;
  utf8 *function;

  u32   line_number;
  u32   hit_count;

  u64   time_stamp;
  u64   high_precision_time;
};

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
internal void meta_log(const char *format, ...);

#define meta_log_char(string) meta_log((utf8 *) (string))
#define meta_log_charf(format, ...) meta_log((utf8 *) (format), __VA_ARGS__)

internal void meta_collate_timing_records(void);

#define TRADER_META_H
#endif
