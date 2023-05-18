#ifndef TRADER_META_IMPL_H
#include "trader_meta.h"

#define TIMED_BLOCK(...) Timed_Block concat(timed_block_, __LINE__)(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

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

  Timed_Block(u32 counter, char *file_name, char *function, u32 line_number, u32 hit_count = 1)
  {
    this->record = timing_records + counter;

    this->record->file_name   = (utf8 *) file_name;
    this->record->function    = (utf8 *) function;
    this->record->line_number = line_number;
    this->record->hit_count  += hit_count;

    this->record->time_stamp          -= platform_get_processor_time_stamp();
    this->record->high_precision_time -= platform_get_high_precision_timer();
  }

  ~Timed_Block()
  {
    this->record->time_stamp          += platform_get_processor_time_stamp();
    this->record->high_precision_time += platform_get_high_precision_timer();
  }
};

#define TRADER_META_IMPL_H
#endif
