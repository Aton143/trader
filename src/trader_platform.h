#ifndef TRADER_PLATFORM_H

struct platform_info
{
  u64 page_size;
};

extern void platform_get_info(platform_info *info);
extern b32 platform_make_memory_pool(u64 page_aligned_size);

#define TRADER_PLATFORM_H
#endif
