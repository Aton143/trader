#ifndef TRADER_META_IMPL_H
#include "trader_meta.h"

const extern u32 timing_records_count;
extern Timing_Record timing_records[];

struct Timed_Block
{
  Timing_Record *record;
  u64            cycle_count_start;
  u64            high_precision_time;
};

internal Timed_Block meta_start_timed_block(u32 counter, char *file_name, char *function, u32 line_number)
{
  Timed_Block res;

  res.record = timing_records + counter;

  res.record->file_name   = (utf8 *) file_name;
  res.record->function    = (utf8 *) function;
  res.record->line_number = line_number;
  res.record->hit_count   = 0;

  res.cycle_count_start   = platform_get_processor_time_stamp();
  res.high_precision_time = platform_get_high_precision_timer();

  return(res);
}

internal void meta_end_timed_block(Timed_Block *timed_block)
{
  timed_block->record->time_stamp +=
    difference_with_wrap(platform_get_processor_time_stamp(), timed_block->cycle_count_start);
  timed_block->record->high_precision_time += platform_get_high_precision_timer() - timed_block->high_precision_time;

  double high_precision_time_in_seconds; high_precision_time_in_seconds =
    platform_convert_high_precision_time_to_seconds(timed_block->high_precision_time);

  timed_block->record->hit_count++;
}

struct Timed_Block_RAII
{
  Timed_Block block;

  Timed_Block_RAII(u32 counter, char *file_name, char *function, u32 line_number)
  {
    this->block = meta_start_timed_block(counter, file_name, function, line_number);
  }

  ~Timed_Block_RAII()
  {
    meta_end_timed_block(&this->block);
  }
};

internal void meta_collate_timing_records(void)
{
  Arena *temp_arena = get_temp_arena();
  Timing_Record *collated_records = push_array_zero(temp_arena, Timing_Record, timing_records_count);

  for (u32 record_index = 0;
       record_index < timing_records_count;
       ++record_index)
  {
    Timing_Record *cur_rec = timing_records + record_index;

    u64 collated_index = ((hash_c_string((char *) cur_rec->function) * cur_rec->line_number) % timing_records_count);;
    Timing_Record *cur_collated = collated_records + collated_index;

    cur_collated->file_name           = cur_rec->file_name;
    cur_collated->function            = cur_rec->function;
    cur_collated->line_number         = cur_rec->line_number;
    cur_collated->hit_count          += cur_rec->hit_count;
    cur_collated->time_stamp          = cur_rec->time_stamp;
    cur_collated->high_precision_time = cur_rec->high_precision_time;
  }

  String_char sprinted_text = push_string(temp_arena, char, 1024);

  for (u32 record_index = 0;
       record_index < timing_records_count;
       ++record_index)
  {
    Timing_Record *cur_collated = collated_records + record_index;

    if (cur_collated->file_name != NULL)
    {
      double high_precision_time_in_seconds =
        platform_convert_high_precision_time_to_seconds(cur_collated->high_precision_time);

      sprinted_text.size = stbsp_snprintf(sprinted_text.str, (int) sprinted_text.cap,
                                         "%s (%lld): %lld cycles (%fs) - %d %s\n",
                                         cur_collated->function,
                                         cur_collated->line_number,
                                         cur_collated->time_stamp,
                                         high_precision_time_in_seconds,
                                         cur_collated->hit_count,
                                         cur_collated->hit_count == 1 ? "time" : "times");

      platform_debug_print(sprinted_text.str);

      sprinted_text.size = 0;
    }
  }

  zero_array(timing_records, Timing_Record, timing_records_count);
}

#define TRADER_META_IMPL_H
#endif
