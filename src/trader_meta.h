#ifndef TRADER_ASSERT_H
#include "trader_handle.h"

#if OS_WINDOWS
# define debug_break() __debugbreak()
#endif

#define assert_break(m) debug_break()
#define assert_always(c) \
  macro_do(              \
   if (!(c))             \
   {                     \
     assert_break(c);    \
     meta_log((utf8 *) "%s failed at line %s in function %s\n", stringify(c), __LINE__, __FUNCTION__); \
   })

#define assert_message_always(m) assert_break(m)

#if !SHIP_MODE
#define assert(c)         assert_always(c)
#define assert_message(m) assert_message_always(m)
#define assert_once(c)                                    \
  local_persist b32 concat(__once__, __LINE__) = false;   \
  if (!concat(__once__, __LINE__) && !(c))                \
    (concat(__once__, __LINE__) = true, assert_break(m))  \

#else
#define assert(c)
#define assert_message(m)
#define assert_once(c)
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

#define TRADER_ASSERT_H
#endif
