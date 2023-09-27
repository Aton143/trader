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

#define __sb_find(op) \
  u32 pos; \
  if (!op((DWORD *) &pos, n)) pos = 0; \
  return(pos);

u32 first_lsb_pos32(u32 n)
{
  __sb_find(_BitScanForward);
}

u32 first_lsb_pos64(u64 n)
{
  __sb_find(_BitScanForward64);
}

u32 first_msb_pos32(u32 n)
{
  __sb_find(_BitScanReverse);
}

u32 first_msb_pos64(u64 n)
{
  __sb_find(_BitScanReverse64);
}

#undef __sb_find

u32 atomic_add32(u32 volatile *addend, u32 value)
{
  u32 res = InterlockedExchangeAdd((LONG volatile *) addend, value);
  return(res);
}

u64 atomic_add64(u64 volatile *addend, u64 value)
{
  u64 res = InterlockedExchangeAdd64((LONG64 volatile *) addend, value);
  return(res);
}

u32 atomic_compare_exchange32(u32 volatile *dest, u32 compare, u32 new_value)
{
  u32 intial_value_at_dest = _InterlockedCompareExchange(dest, new_value, compare);
  return(intial_value_at_dest);
}

u64 atomic_compare_exchange64(u64 volatile *dest, u64 compare, u64 new_value)
{
  u64 initial_value_at_dest = _InterlockedCompareExchange64((volatile LONG64 *) dest, new_value, compare);
  return(initial_value_at_dest);
}

#define CL_IMPL_H
#endif
