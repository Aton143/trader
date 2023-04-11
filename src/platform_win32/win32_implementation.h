#ifndef WIN32_IMPLEMENTATION_H

void platform_get_info(platform_info *info)
{
  SYSTEM_INFO win32_info = {};
  GetSystemInfo(&win32_info);

  info->page_size = (u64) win32_info.dwPageSize;
}

b32 platform_make_memory_pool(u64 page_aligned_size)
{
  b32 result = true;

  u8 *memory_returned = (u8 *) VirtualAlloc(NULL, page_aligned_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (memory_returned != NULL)
  {
    global_memory_pool.start                = memory_returned;
    global_memory_pool.size                 = page_aligned_size;
    global_memory_pool.arena_free_list_head = global_arena_list;
  }
  else
  {
    result = false;
  }

  return(result);
}

#define WIN32_IMPLEMENTATION_H
#endif
