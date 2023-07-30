#include <stdlib.h>
#include "trader_memory.h"

internal inline i64 copy_memory_block(void *dest, void *source, i64 byte_count)
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

internal inline i64 set_memory_block(void *dest, u8 val, i64 byte_count)
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

internal inline i64 move_memory_block(void *dest, void *source, i64 byte_count)
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

internal inline i64 compare_memory_block(void *a, void *b, i64 byte_count)
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

internal inline void *arena_push(Arena *arena, u64 size)
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

internal inline void *arena_push_zero(Arena *arena, u64 size)
{
  void *memory_given_back = arena_push(arena, size);

  if (memory_given_back)
  {
    zero_memory_block(memory_given_back, size);
  }

  return(memory_given_back);
}

internal Arena push_arena(Arena *arena, u64 size, u64 alignment)
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

internal inline void arena_pop(Arena *arena, u64 size)
{
  arena->used -= size;
}

internal inline void *arena_append(Arena *arena, void *data, u64 size)
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

internal inline u64 arena_get_remaining_size(Arena *arena)
{
  expect(arena != NULL);
  u64 remaining_size = arena->size - arena->used;
  return(remaining_size);
}

internal inline void *_arena_get_top(Arena *arena, u64 size)
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

internal inline Ring_Buffer ring_buffer_make(Arena *arena, u64 size)
{
  Ring_Buffer ring_buffer = {};

  u8 *start = (u8 *) arena_push(arena, size);
  if (start != NULL)
  {
    ring_buffer.start = ring_buffer.write = ring_buffer.read = start;
    ring_buffer.size  = size;
  }
  else
  {
    expect_message(false, "expected to successfully make ring buffer");
  }

  return(ring_buffer);
}

internal inline void ring_buffer_reset(Ring_Buffer *rb)
{
  rb->read = rb->write = rb->start;
}

internal inline void *ring_buffer_push(Ring_Buffer *rb, u64 size)
{
  expect_message(((rb->size % size) == 0), "expected ring buffer size to be a multiple of input");

  void *result = (void *) rb->write;

  ptr_val rel_write_pos = ((ptr_val) rb->write) - ((ptr_val) rb->start);
  rel_write_pos = (rel_write_pos + size) % rb->size;

  rb->write = rb->start + rel_write_pos;

  return(result);
}

internal inline void *ring_buffer_append(Ring_Buffer *rb, void *data, u64 size)
{
  void *result = ring_buffer_push(rb, size);
  copy_memory_block(result, data, size);
  return(result);
}

internal inline void *ring_buffer_pop(Ring_Buffer *rb, u64 size)
{
  expect_message(((rb->size % size) == 0), "expcted ring buffer size to be a multiple of input");

  void *result = (void *) rb->read;

  ptr_val rel_read_pos = ((ptr_val) rb->read) - ((ptr_val) rb->start);
  rel_read_pos = (rel_read_pos + size) % rb->size;

  rb->read = rb->start + rel_read_pos;

  return(result);
}

internal inline void ring_buffer_pop_and_put(Ring_Buffer *rb,
                                             void       *data,
                                             u64         size)
{
  void *read = ring_buffer_pop(rb, size);
  copy_memory_block(data, read, size);
}

internal inline Arena *get_temp_arena(Thread_Context *context)
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

internal Arena arena_alloc(u64 size, u64 alignment, void *start)
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

internal inline Arena get_rest_of_temp_arena(f32 rest, Thread_Context *context)
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

internal inline u64 get_temp_arena_used(Thread_Context *context)
{
  Temp_Arena *temp_arena = &context->local_temp_arena;
  return(temp_arena->arena.used);
}

internal void set_temp_arena_wait(u64 wait, Thread_Context *context)
{
  context->local_temp_arena.wait = wait;
}

