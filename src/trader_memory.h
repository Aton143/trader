#ifndef TRADER_MEMORY_H

// TODO(antonio): store_ptr/load_ptr versions
// i.e. ring_buffer_append(..., &widget) caused issues in the past

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

struct Thread_Context
{
  Temp_Arena local_temp_arena;
};

global_const u32            thread_count = 2;
global       Thread_Context thread_contexts[thread_count] = {};

#if SHIP_MODE
global_const void *global_memory_start_addr = NULL;
#else
global_const void *global_memory_start_addr = (void *) tb(2);
#endif

global_const u64 global_memory_size = mb(8);
global_const u64 global_temp_arena_size = mb(32);

internal inline i64 copy_memory_block(void *dest, void *source, i64 byte_count);
#define copy_string(dest, string)       copy_memory_block((dest), (string).str, (string).size)
#define copy_string_lit(dest, string)   copy_memory_block((dest), (string), sizeof(string))
#define copy_struct(dest, copy)         copy_memory_block(dest, copy, sizeof(*(copy)))
#define copy_array(dest, array, length) copy_memory_block(dest, array, sizeof(*(array) * length))

internal inline i64 set_memory_block(void *dest, u8 val, i64 byte_count);
#define zero_memory_block(dest, byte_count) set_memory_block((void *) (dest), 0, (byte_count))
#define zero_array(dest, type, count) zero_memory_block((dest), sizeof(type)*(count))
#define zero_struct(dest) zero_memory_block((dest), sizeof(*(dest)))
#define zero_literal(literal) zero_memory_block(literal, sizeof(literal))

internal inline i64 move_memory_block(void *dest, void *source, i64 byte_count);

internal inline i64 compare_memory_block(void *a, void *b, i64 byte_count);
#define compare_struct_shallow(a, b) compare_memory_block(a, b, sizeof(*a))

internal inline Arena arena_alloc(u64 size, u64 alignment, void *start);
unimplemented void arena_release(Arena *arena);

internal inline Arena *get_temp_arena(Thread_Context *context = thread_contexts);
internal inline Arena  get_rest_of_temp_arena(f32 rest = 1.0f, Thread_Context *context = thread_contexts);
internal inline u64    get_temp_arena_used(Thread_Context *context = thread_contexts);
internal inline void   set_temp_arena_used(u64 size, Thread_Context *context = thread_contexts);
internal inline void   set_temp_arena_wait(u64 wait, Thread_Context *context = thread_contexts);
         
internal inline void *arena_push(Arena *arena, u64 size);
internal inline void *arena_push_zero(Arena *arena, u64 size);
         
internal inline Arena push_arena(Arena *arena, u64 size);

#define push_array(arena, type, count) (type *) arena_push((arena), sizeof(type)*(count))
#define push_array_zero(arena, type, count) (type *) arena_push_zero((arena), sizeof(type)*(count))
#define push_buffer(arena, size) {(u8 * ) arena_push(arena, size), size}

#define push_string(arena, char_type, cap) {push_array(arena, char_type, cap), 0, cap*sizeof(char_type)}
#define push_string_zero(arena, char_type, cap) {push_array_zero(arena, char_type, cap), 0, cap*sizeof(char_type)}

#define push_struct(arena, type) (type *) arena_push((arena), sizeof(type))
#define push_struct_zero(arena, type) (type *) arena_push_zero((arena), sizeof(type))

internal inline void *arena_append(Arena *arena, void *data, u64 size);
#define append_struct(arena, element) arena_append(arena, element, sizeof(*element))

internal inline void arena_pop(Arena *arena, u64 size);
#define arena_reset(arena) arena_pop((arena), (arena)->used)
#define arena_reset_zero(arena) zero_memory_block((arena)->start, (arena)->used); arena_pop((arena), (arena)->used)

#define pop_array(arena, type, count) arena_pop((arena), sizeof(type)*(count))
#define pop_struct(arena, type) arena_pop((arena), sizeof(type))

internal      inline u64 arena_get_remaining_size(Arena *arena);
unimplemented inline u64 arena_get_pos(Arena *arena);

unimplemented inline void arena_set_pos_back(Arena *arena, u64 pos);
unimplemented inline void arena_clear(Arena *arena);

internal inline void *_arena_get_top(Arena *arena, u64 size);
#define arena_get_top(arena, type) (type *) _arena_get_top((arena), sizeof(type))

#if OS_WINDOWS
#define alloca _alloca
#endif

#define stack_alloc(bytes) alloca(bytes)
#define stack_alloc_buffer(byte_count) {(u8 *) alloca(byte_count), byte_count, 0}
#define zero_buffer(buf) zero_memory_block((buf)->data, (buf)->size)

// NOTE(antonio): write is ahead of read
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
internal inline Ring_Buffer ring_buffer_make(Arena *arena, u64 size);
internal inline void        ring_buffer_reset(Ring_Buffer *rb);

internal inline void *ring_buffer_push(Ring_Buffer *ring_buffer, u64 size);
#define ring_buffer_push_struct(rb, type)       \
  (type *) ring_buffer_push((rb), sizeof(type))
internal inline void *ring_buffer_append(Ring_Buffer *ring_buffer,
                                         void        *data,
                                         u64          size);

internal inline void *ring_buffer_pop(Ring_Buffer *ring_buffer, u64 size);
#define ring_buffer_pop_struct(rb, type) \
  (type *) ring_buffer_pop((rb), sizeof(type))

internal inline void ring_buffer_pop_and_put(Ring_Buffer *ring_buffer,
                                             void        *data,
                                             u64          size);
#define ring_buffer_pop_and_put_struct(rb, copy) ring_buffer_pop_and_put(rb, copy, sizeof(*(copy)))

internal inline u16 byte_swap_16(u16 val);
internal inline u32 byte_swap_32(u32 val);
internal inline u64 byte_swap_64(u64 val);

// implementation
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

#define TRADER_MEMORY_H
#endif
