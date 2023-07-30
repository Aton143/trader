#ifndef CL_IMPL_H
#include <stdlib.h>

#define align_data(bytes) __declspec(align(bytes))

inline internal u16 byte_swap_16(u16 val)
{
  u16 swapped = _byteswap_ushort(val);
  return(swapped);
}

inline internal u32 byte_swap_32(u32 val)
{
  u32 swapped = _byteswap_ulong(val);
  return(swapped);
}

inline internal u64 byte_swap_64(u64 val)
{
  u64 swapped = _byteswap_uint64(val);
  return(swapped);
}

#define CL_IMPL_H
#endif
