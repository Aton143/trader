#ifndef CL_IMPL_H

#include <stdlib.h>
#include "../trader_utils.h"

#define align_data(bytes) __declspec(align(bytes))

u16 byte_swap_16(u16 val)
{
  u16 swapped = _byteswap_ushort(val);
  return(swapped);
}

u32 byte_swap_32(u32 val)
{
  u32 swapped = _byteswap_ulong(val);
  return(swapped);
}

u64 byte_swap_64(u64 val)
{
  u64 swapped = _byteswap_uint64(val);
  return(swapped);
}

u64 get_processor_time_stamp(void)
{
  u64 processor_ts = __rdtsc();
  return(processor_ts);
}

u16 popcount16(u16 n)
{
  u16 res = __popcnt16(n);
  return(res);
}

u32 popcount32(u32 n)
{
  u32 res = __popcnt(n);
  return(res);
}

u64 popcount64(u64 n)
{
  u64 res = __popcnt64(n);
  return(res);
}

#define CL_IMPL_H
#endif
