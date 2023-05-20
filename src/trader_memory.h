#ifndef TRADER_MEMORY_H

#if SHIP_MODE
global_const void *global_memory_start_addr = NULL;
#else
global_const void *global_memory_start_addr = (void *) tb(2);
#endif

global_const u64 global_memory_size = mb(8);
global_const u64 global_temp_arena_size = kb(512);

internal i64 copy_memory_block(void *dest, void *source, i64 byte_count);
#define copy_string(dest, string)       copy_memory_block((dest), (string).str, (string).size)
#define copy_string_lit(dest, string)   copy_memory_block((dest), (string), sizeof(string))
#define copy_struct(dest, copy)         copy_memory_block(dest, copy, sizeof(*(copy)))
#define copy_array(dest, array, length) copy_memory_block(dest, array, sizeof(*(array) * length))

internal i64 set_memory_block(void *dest, u8 val, i64 byte_count);
#define zero_memory_block(dest, byte_count) set_memory_block((void *) (dest), 0, (byte_count))
#define zero_struct(dest) zero_memory_block((dest), sizeof(*(dest)))
#define zero_literal(literal) zero_memory_block(literal, sizeof(literal))

internal i64 move_memory_block(void *dest, void *source, i64 byte_count);

internal i64 compare_memory_block(void *a, void *b, i64 byte_count);
#define compare_struct_shallow(a, b) compare_memory_block(a, b, sizeof(*a))

struct Arena
{
  u8   *start;
  u64   size;
  u64   used;
  u64   alignment;
};

struct Temp_Arena
{
  Arena arena;
  u64   wait;
};

internal Arena arena_alloc(u64 size, u64 alignment, void *start);
unimplemented void arena_release(Arena *arena);
internal void arena_reset(Arena *arena);

internal Arena *get_temp_arena(void);
internal void   set_temp_arena_wait(u64 wait);

internal void *arena_push(Arena *arena, u64 size);
internal void *arena_push_zero(Arena *arena, u64 size);

#define push_array(arena, type, count) (type *) arena_push((arena), sizeof(type)*(count))
#define push_array_zero(arena, type, count) (type *) arena_push_zero((arena), sizeof(type)*(count))
#define push_buffer(arena, size) {(u8 * ) arena_push(arena, size), size}
#define push_string(arena, char_type, cap) {push_array(arena, char_type, cap), 0, cap}

#define push_struct(arena, type) (type *) arena_push((arena), sizeof(type))
#define push_struct_zero(arena, type) (type *) arena_push_zero((arena), sizeof(type))

internal void *arena_append(Arena *arena, void *data, u64 size);
#define append_struct(arena, element) arena_append(arena, element, sizeof(*element))

internal void arena_pop(Arena *arena, u64 size);
#define arena_reset(arena) arena_pop((arena), (arena)->used)

#define pop_array(arena, type, count) arena_pop((arena), sizeof(type)*(count))
#define pop_struct(arena, type) arena_pop((arena), sizeof(type))

unimplemented u64 arena_get_pos(Arena *arena);

unimplemented void arena_set_pos_back(Arena *arena, u64 pos);
unimplemented void arena_clear(Arena *arena);

internal void *_arena_get_top(Arena *arena, u64 size);
#define arena_get_top(arena, type) (type *) _arena_get_top((arena), sizeof(type))

struct Ring_Buffer
{
  u8 *start;

  union
  {
    u8 *read;
    u8 *front;
  };

  union
  {
    u8 *write;
    u8 *back;
  };

  u64 size;
};

// TODO(antonio): is an arena the best place to get the memory from?
internal Ring_Buffer ring_buffer_make(Arena *arena, u64 size);

internal void *ring_buffer_push(Ring_Buffer *ring_buffer, u64 size);
#define ring_buffer_push_struct(rb, type)       \
  (type *) ring_buffer_push((rb), sizeof(type))
internal void *ring_buffer_append(Ring_Buffer *ring_buffer,
                                  void        *data,
                                  u64          size);

internal void *ring_buffer_pop(Ring_Buffer *ring_buffer, u64 size);
#define ring_buffer_pop_struct(rb, type) \
  (type *) ring_buffer_pop((rb), sizeof(type))

internal void ring_buffer_pop_and_put(Ring_Buffer *ring_buffer,
                                      void        *data,
                                      u64          size);
#define ring_buffer_pop_and_put_struct(rb, copy) ring_buffer_pop_and_put(rb, copy, sizeof(*(copy)))

// implementation
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
  else if ((to > from) && ((to - from) < (i32) byte_count))
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
  }
  else if ((from > to) && ((from - to) < (i32) byte_count))
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

void ring_buffer_pop_and_put(Ring_Buffer *rb,
                              void       *data,
                              u64         size)
{
  void *read = ring_buffer_pop(rb, size);
  copy_memory_block(data, read, size);
}

#define TRADER_MEMORY_H
#endif
