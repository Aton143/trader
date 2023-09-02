#ifndef GCC_IMPL_H

#define align_data(bytes) __attribute__((aligned(bytes)))

u16 byte_swap_16(u16 val)
{
  u16 swapped = __builtin_bswap16(val);
  return(swapped);
}

u32 byte_swap_32(u32 val)
{
  u32 swapped = __builtin_bswap32(val);
  return(swapped);
}

u64 byte_swap_64(u64 val)
{
  u64 swapped = __builtin_bswap64(val);
  return(swapped);
}

u64 get_processor_time_stamp(void)
{
#if ARCH_X64
  u32 tick_lo, tick_hi;
  __asm__ __volatile__("rdtsc" : "=a"(tick_lo), "=d"(tick_hi));
  u64 processor_ts = (((u64) tick_hi) << 32) | ((u64) tick_lo);
  return(processor_ts);
#endif
}

u16 popcount16(u16 n)
{
  u16 res = __builtin_popcount((unsigned int) n);
  return(res);
}

u32 popcount32(u32 n)
{
  u32 res = __builtin_popcount((unsigned int) n);
  return(res);
}

u64 popcount64(u64 n)
{
  u64 res = __builtin_popcountll((unsigned long long) n);
  return(res);
}

#define GCC_IMPL_H
#endif
