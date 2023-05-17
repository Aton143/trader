#ifndef TRADER_META_IMPL_H
#include "trader_meta.h"

internal void _start_timing_block(utf8 *block_name, u64 block_name_size)
{
  unused(block_name);
  unused(block_name_size);

  meta_info.last_time_stamp          = platform_get_processor_time_stamp();
  meta_info.last_high_precision_time = platform_get_high_precision_timer();
}

internal void _end_timing_block(void)
{
  u64 time_stamp_diff =
    difference_with_wrap(platform_get_processor_time_stamp(), meta_info.last_time_stamp);
  unused(time_stamp_diff);

  u64 high_precision_time_diff =
    difference_with_wrap(platform_get_high_precision_timer(), meta_info.last_high_precision_time);
  unused(high_precision_time_diff);
}

#define TRADER_META_IMPL_H
#endif
