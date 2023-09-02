#ifndef CLANG_IMPL_H

#define align_data(bytes) __attribute__((aligned(bytes)))

inline internal u16 byte_swap_16(u16 val)
{
  u16 swapped = __builtin_bswap16(val);
  return(swapped);
}

inline internal u32 byte_swap_32(u32 val)
{
  u32 swapped = __builtin_bswap32(val);
  return(swapped);
}

inline internal u64 byte_swap_64(u64 val)
{
  u64 swapped = __builtin_bswap64(val);
  return(swapped);
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

#define CLANG_IMPL_H
#endif
