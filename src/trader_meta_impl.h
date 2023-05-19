#ifndef TRADER_META_IMPL_H
#include "trader_meta.h"

#define TIMED_BLOCK() Timed_Block \
  concat(timed_block_, __LINE__)(__COUNTER__, __FILE__, __FUNCTION__, __LINE__)
#define TIMED_BLOCK_START(...) Timed_Block \
  __timed_block__(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define TIMED_BLOCK_END() __timed_block__.~Timed_Block()

struct Timing_Record
{
  utf8 *file_name;
  utf8 *function;

  u32   line_number;
  u32   hit_count;

  u64   time_stamp;
  u64   high_precision_time;
};

const extern u32 timing_records_count;
extern Timing_Record timing_records[];

struct Timed_Block
{
  Timing_Record *record;
  u64            cycle_count_start;
  u64            high_precision_time;

  Timed_Block(u32 counter, char *file_name, char *function, u32 line_number)
  {
    this->record = timing_records + counter;

    this->record->file_name   = (utf8 *) file_name;
    this->record->function    = (utf8 *) function;
    this->record->line_number = line_number;

    this->cycle_count_start   = platform_get_processor_time_stamp();
    this->high_precision_time = platform_get_high_precision_timer();
  }

  ~Timed_Block()
  {
    this->record->time_stamp          += difference_with_wrap(platform_get_processor_time_stamp(), this->cycle_count_start);
    this->record->high_precision_time += difference_with_wrap(platform_get_high_precision_timer(), this->high_precision_time);
    this->record->hit_count++;
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
        ((double) cur_collated->high_precision_time) / ((double) meta_info.high_precision_timer_frequency);

      sprinted_text.size = stbsp_snprintf(sprinted_text.str, (int) sprinted_text.cap,
                                         "%s (%lld): %lld cycles (%fs) - %lld times\n",
                                         cur_collated->function,
                                         cur_collated->line_number,
                                         cur_collated->time_stamp,
                                         high_precision_time_in_seconds,
                                         cur_collated->hit_count);

      OutputDebugStringA(sprinted_text.str);

      sprinted_text.size = 0;
    }
  }
}

#define TRADER_META_IMPL_H
#endif
