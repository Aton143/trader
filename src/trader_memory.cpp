#include <stdlib.h>
#include "trader_memory.h"

i64 copy_memory_block(void *dest, void *source, i64 byte_count)
{
  u8 *dest_bytes = (u8 *) dest;
  u8 *source_bytes = (u8 *) source;

  i64 byte_index;
  for (byte_index = 0; byte_index < byte_count; ++byte_index)
  {
    dest_bytes[byte_index] = source_bytes[byte_index];
  }

  return(byte_index);
}

i64 set_memory_block(void *dest, u8 val, i64 byte_count)
{
  u8 *dest_bytes = (u8 *) dest;

  i64 byte_index;
  for (byte_index = 0;
       byte_index < byte_count;
       ++byte_index)
  {
    dest_bytes[byte_index] = val;
  }

  return(byte_index);
}

i64 move_memory_block(void *dest, void *source, i64 byte_count)
{
  u8 *from = (u8 *) source;
  u8 *to   = (u8 *) dest;

  if ((from == to) || (byte_count == 0))
  {
  }

  if ((to > from) && ((to - from) < (i32) byte_count))
  {
    /* to overlaps with from */
    /*  <from......>         */
    /*         <to........>  */
    /* copy in reverse, to avoid overwriting from */
    i64 i;
    for (i = byte_count - 1;
         i >= 0;
         i--)
    {
      to[i] = from[i];
    }
    return(byte_count);
  }

  if ((from > to) && ((from - to) < (i32) byte_count))
  {
    /* to overlaps with from */
    /*        <from......>   */
    /*  <to........>         */
    /* copy forwards, to avoid overwriting from */
    i64 i;
    for (i = 0;
         i < byte_count;
         i++)
    {
      to[i] = from[i];
    }
    return(byte_count);
  }

  copy_memory_block(dest, source, byte_count);
  return(byte_count);
}

i64 compare_memory_block(void *a, void *b, i64 byte_count)
{
  i64 result = 0;

  u8 *a_bytes = (u8 *) a;
  u8 *b_bytes = (u8 *) b;

  expect(a_bytes != NULL);
  expect(b_bytes != NULL);

  for (i64 byte_index = 0;
       byte_index < byte_count;
       ++byte_index)
  {
    if (a_bytes[byte_index] != b_bytes[byte_index]) 
    {
      result = 1;
    }
  }

  return(result);
}

void *arena_push(Arena *arena, u64 size)
{
  void *memory_given_back = NULL;

  size = align(size, arena->alignment);
  if ((arena->used + size) <= arena->size)
  {
    void *aligned_ptr = (void *) align((ptr_val) (arena->start + arena->used), arena->alignment);
    memory_given_back = aligned_ptr;

    arena->used += size;
  }

  return(memory_given_back);
}

void *arena_push_zero(Arena *arena, u64 size)
{
  void *memory_given_back = arena_push(arena, size);

  if (memory_given_back)
  {
    zero_memory_block(memory_given_back, size);
  }

  return(memory_given_back);
}

Arena push_arena(Arena *arena, u64 size, u64 alignment)
{
  expect(arena != NULL);
  expect((arena->size - arena->used) >= size);

  Arena created_arena;

  created_arena.start = (u8 *) arena_push(arena, size);
  created_arena.used  = 0;
  created_arena.size  = size;
  created_arena.alignment  = alignment;

  expect(created_arena.start != NULL);
  return(created_arena);
}

void arena_pop(Arena *arena, u64 size)
{
  arena->used -= size;
}

void *arena_append(Arena *arena, void *data, u64 size)
{
  void *result = NULL;

  if ((arena->used + size) <= arena->size)
  {
    result = arena->start + arena->used;
    copy_memory_block(result, data, size);
    arena->used += size;
  }
  else
  {
    expect_message(false, "this should work, asshole");
  }

  return(result);
}

u64 arena_get_remaining_size(Arena *arena)
{
  expect(arena != NULL);
  u64 remaining_size = arena->size - arena->used;
  return(remaining_size);
}

void *_arena_get_top(Arena *arena, u64 size)
{
  void *result = NULL;

  i64 top_start = (i64) (arena->used - size);
  if (top_start >= 0)
  {
    expect(((u64) top_start) < arena->size);
    result = arena->start + top_start;
  }

  return(result);
}

Ring_Buffer ring_buffer_make(Arena *arena, u64 size)
{
  Ring_Buffer ring_buffer;

  u8 *start = (u8 *) arena_push(arena, size);
  ring_buffer = ring_buffer_make(start, size);

  return(ring_buffer);
}

Ring_Buffer ring_buffer_make(u8 *buffer, u64 size)
{
  expect(buffer != NULL);
  Ring_Buffer ring_buffer = {};
  ring_buffer.start = ring_buffer.write = ring_buffer.read = buffer;
  ring_buffer.size  = size;
  return(ring_buffer);
}

void ring_buffer_reset(Ring_Buffer *rb)
{
  rb->read = rb->write = rb->start;
}

void *ring_buffer_push(Ring_Buffer *rb, u64 size)
{
  expect_message(((rb->size % size) == 0), "expected ring buffer size to be a multiple of input");

  void *result = (void *) rb->write;

  ptr_val rel_write_pos = ((ptr_val) rb->write) - ((ptr_val) rb->start);
  rel_write_pos = (rel_write_pos + size) % rb->size;

  rb->write = rb->start + rel_write_pos;

  return(result);
}

void *ring_buffer_append(Ring_Buffer *rb, void *data, u64 size)
{
  void *result = ring_buffer_push(rb, size);
  copy_memory_block(result, data, size);
  return(result);
}

void *ring_buffer_pop(Ring_Buffer *rb, u64 size)
{
  expect_message(((rb->size % size) == 0), "expcted ring buffer size to be a multiple of input");

  void *result = (void *) rb->read;

  ptr_val rel_read_pos = ((ptr_val) rb->read) - ((ptr_val) rb->start);
  rel_read_pos = (rel_read_pos + size) % rb->size;

  rb->read = rb->start + rel_read_pos;

  return(result);
}

void ring_buffer_pop_and_put(Ring_Buffer *rb, void *data, u64 size)
{
  void *read = ring_buffer_pop(rb, size);
  copy_memory_block(data, read, size);
}

Arena *get_temp_arena(Thread_Context *context)
{
  Temp_Arena *temp_arena = &context->local_temp_arena;

  if (temp_arena->wait > 0)
  {
    temp_arena->wait--;
  }
  else
  {
    temp_arena->arena.used = 0;
  }

  return(&temp_arena->arena);
}

Arena arena_alloc(u64 size, u64 alignment, void *start)
{
  Arena arena = {};

  u8 *allocated = platform_allocate_memory_pages(size, start);

  expect(allocated != NULL);

  arena.start     = allocated;
  arena.size      = size;
  arena.used      = 0;
  arena.alignment = alignment;

  return(arena);
}

Arena arena_make(void *start, u64 size, u64 alignment)
{
  Arena arena = {};

  expect(start != NULL);
  expect(size > 0);
  expect(popcount64(alignment) == 1);

  arena.start     = (u8 *) start;
  arena.used      = 0;
  arena.size      = size;
  arena.alignment = alignment;
    
  return(arena);
}

Arena get_rest_of_temp_arena(f32 rest, Thread_Context *context)
{
  expect(is_between_inclusive(0.0f, rest, 1.0f));
  expect(context != NULL);

  Temp_Arena *temp_arena = &context->local_temp_arena;

  Arena res;
  res.start     = temp_arena->arena.start + temp_arena->arena.used;
  res.size      = (u64) (rest * (temp_arena->arena.size  - temp_arena->arena.used));
  res.used      = 0;
  res.alignment = temp_arena->arena.alignment;

  temp_arena->arena.used += res.size;
  expect(temp_arena->arena.used < temp_arena->arena.size);

  return(res);
}

u64 get_temp_arena_used(Thread_Context *context)
{
  Temp_Arena *temp_arena = &context->local_temp_arena;
  return(temp_arena->arena.used);
}

void set_temp_arena_wait(u64 wait, Thread_Context *context)
{
  context->local_temp_arena.wait = wait;
}

Bucket_List bucket_list_make(void              *memory,
                             u64                total_size,
                             u32                bucket_max_size,
                             u16                header_size,
                             u16                alignment,
                             String_Const_utf8  tag)
{
  Bucket_List bucket_meta_info = {};
  u8 *mem = (u8 *) memory;

  expect(memory != NULL);
  expect((total_size > 0));
  expect((total_size % bucket_max_size) == 0); // can be relaxed
  expect((bucket_max_size % alignment) == 0);
  expect(popcount16(alignment) == 1);

  bucket_meta_info.tag             = tag;

  bucket_meta_info.memory          = mem;
  bucket_meta_info.total_size      = total_size;
  bucket_meta_info.bucket_max_size = bucket_max_size;

  bucket_meta_info.header_size     = header_size;
  bucket_meta_info.alignment       = alignment;

  u32 bucket_count = (u32) (total_size / bucket_max_size);
  Bucket *cur;

  for (u32 bucket_id = 0;
       bucket_id < (bucket_count - 1);
       ++bucket_id)
  {
    cur = (Bucket *) mem;
    cur->next_bucket_id = bucket_id + 1;
    mem += bucket_max_size;
  }

  cur = (Bucket *) mem;
  cur->next_bucket_id = bucket_list_invalid_id;

  bucket_meta_info.first_bucket_id = bucket_list_invalid_id;
  bucket_meta_info.last_bucket_id  = bucket_list_invalid_id;

  bucket_meta_info.next_available_id = 0;
  bucket_meta_info.cur_count         = 0;

  return(bucket_meta_info);
}

Bucket_List bucket_list_make(Arena             *arena,
                             u64                total_size,
                             u32                bucket_max_size,
                             u16                header_size,
                             u16                alignment,
                             String_Const_utf8  tag)
{
  u8 *memory = (u8 *) arena_push(arena, total_size);
  Bucket_List meta = bucket_list_make(memory, total_size, bucket_max_size, header_size, alignment, tag);
  return(meta);
}

Bucket *bucket_list_get_from_id(Bucket_List *meta, u32 id)
{
  Bucket *res = NULL;

  expect(meta != NULL);

  if (id != bucket_list_invalid_id)
  {
    res = (Bucket *) (meta->memory + (id * meta->bucket_max_size));
    expect((uintptr_t) res < ((uintptr_t) (meta->memory + meta->total_size)));
  }

  return(res);
}

Bucket *bucket_list_get_first(Bucket_List *meta)
{
  Bucket *first = bucket_list_get_from_id(meta, meta->first_bucket_id);
  return(first);
}

Bucket *bucket_list_get_new_and_update(Bucket_List *meta, u32 data_size)
{
  Bucket *res = bucket_list_get_from_id(meta, meta->next_available_id);
  u32 id = bucket_list_get_id(meta, res);

  if (res != NULL)
  {
    meta->next_available_id = res->next_bucket_id;
    meta->cur_count++;

    if (meta->first_bucket_id == bucket_list_invalid_id)
    {
      meta->first_bucket_id = id;
    }

    if (meta->last_bucket_id != bucket_list_invalid_id)
    {
      bucket_list_get_from_id(meta, meta->last_bucket_id)->next_bucket_id = id;
    }

    meta->last_bucket_id = id;
    res->next_bucket_id  = bucket_list_invalid_id;
    res->data_size       = data_size;
  }
  else
  {
    expect(meta->next_available_id == bucket_list_invalid_id);
  }

  return(res);
}

internal inline u32 bucket_list_get_id(Bucket_List *meta, Bucket *bucket)
{
  u32 id;

  if (bucket != NULL) 
  {
    id = (u32) ((((uintptr_t) bucket) - ((uintptr_t) meta->memory)) / meta->bucket_max_size);
  }
  else
  {
    id = bucket_list_invalid_id;
  }

  return(id);
}

void bucket_list_print(Bucket_List *list)
{
  Bucket *cur = bucket_list_get_first(list);

  platform_debug_print("\nListing active buckets:\n");
  while (cur != NULL)
  {
    float *header = (float *) bucket_list_get_header_start(list, cur);
    platform_debug_printf("bucket id: %d (%s)\n",
                          bucket_list_get_id(list, cur),
                          *header > 0.0f ? "Keep" : "Put Away");
    cur = bucket_list_get_from_id(list, cur->next_bucket_id);
  }

  cur = bucket_list_get_from_id(list, list->next_available_id);

  platform_debug_print("\nListing inactive buckets:\n");
  while (cur != NULL)
  {
    platform_debug_printf("bucket id: %d\n", bucket_list_get_id(list, cur));
    cur = bucket_list_get_from_id(list, cur->next_bucket_id);
  }
}

void bucket_list_put_back(Bucket_List *meta, Pair_u32 *ids, u32 count)
{
  expect(meta->cur_count >= count);

  if (count == 0) return;

  u32 id_index = 0;
  while (id_index < count)
  {
    u32 range_start = id_index;
    u32 range_end   = range_start;

    u32 ends_next = bucket_list_get_from_id(meta, ids[range_end].cur)->next_bucket_id;

    while ((range_end + 1) < count)
    {
      u32 next_id = ids[range_end + 1].cur;
      ends_next   = bucket_list_get_from_id(meta, ids[range_end].cur)->next_bucket_id;

      if (ends_next == next_id)
      {
        range_end++;
      }
      else
      {
        break;
      }
    }

    Bucket *start_prev = bucket_list_get_from_id(meta, ids[range_start].prev);

    u32     end_next_id = bucket_list_get_from_id(meta, ids[range_end].cur)->next_bucket_id;
    Bucket *end_next    = bucket_list_get_from_id(meta, end_next_id);

    if ((start_prev != NULL) && (end_next != NULL))
    {
      start_prev->next_bucket_id = end_next_id;
    }
    else if ((start_prev == NULL) && (end_next != NULL)) // first in range is first of bucket list
    {
      local_persist i32 c = 0;
      c++;
      meta->first_bucket_id = end_next_id;
    }
    else if ((start_prev != NULL) && (end_next == NULL)) // got to end of list
    {
      start_prev->next_bucket_id = bucket_list_invalid_id;
    }
    else if ((start_prev == NULL) && (end_next == NULL)) // entire list
    {
      meta->first_bucket_id = bucket_list_invalid_id;
      meta->last_bucket_id  = bucket_list_invalid_id;
    }

    bucket_list_get_from_id(meta, ids[range_end].cur)->next_bucket_id = meta->next_available_id;
    meta->next_available_id = ids[range_start].cur;

    id_index = range_end + 1;
  }

  meta->cur_count -= count;
}

void *bucket_list_get_header_start(Bucket_List *meta, Bucket *bucket)
{
  u8 *res;

  expect(meta != NULL);

  u32 bucket_start_size = member_size(Bucket, next_bucket_id) + member_size(Bucket, data_size);
  u32 header_start      = align(bucket_start_size, meta->alignment);
  res = header_start + ((u8 *) bucket);

  return(res);
}

void *bucket_list_get_data_start(Bucket_List *meta, Bucket *bucket)
{
  u8 *res;

  expect(meta != NULL);

  u32 bucket_start_size = member_size(Bucket, next_bucket_id) + member_size(Bucket, data_size);
  u32 header_start      = align(bucket_start_size, meta->alignment);
  u32 data_start        = align(header_start + meta->header_size, meta->alignment);

  res = ((u8 *) bucket) + data_start;
  return(res);
}

u32 bucket_list_get_count_fits_in_data(Bucket_List *meta, u32 size)
{
  u32 count;

  u32 data_size = meta->bucket_max_size - (u32) ((uintptr_t) bucket_list_get_data_start(meta, NULL));
  count = data_size / size;

  return(count);
}
