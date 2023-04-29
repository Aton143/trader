#ifndef TRADER_MEMORY_H

#if SHIP_MODE
global_const void *global_memory_start_addr = NULL;
#else
global_const void *global_memory_start_addr = (void *) tb(2);
#endif

global_const u64 global_memory_size = mb(8);

struct Arena
{
  u8   *start;
  u64   size;
  u64   used;
  u64   alignment;
};

internal Arena arena_alloc(u64 size, u64 alignment, void *start);
unimplemented void arena_release(Arena *arena);

void *arena_push(Arena *arena, u64 size);
void *arena_push_zero(Arena *arena, u64 size);

#define push_array(arena, type, count) (type *) arena_push((arena), sizeof(type)*(count))
#define push_array_zero(arena, type, count) (type *) arena_push_zero((arena), sizeof(type)*(count))
#define push_buffer(arena, size) {(u8 * ) arena_push(arena, size), size}

#define push_struct(arena, type) (type *) arena_push((arena), sizeof(type))
#define push_struct_zero(arena, type) (type *) arena_push_zero((arena), sizeof(type))

void arena_pop(Arena *arena, u64 size);

#define pop_array(arena, type, count) arena_pop((arena), sizeof(type)*(count))
#define pop_struct(arena, type) arena_pop((arena), sizeof(type))

unimplemented u64 arena_get_pos(Arena *arena);

unimplemented void arena_set_pos_back(Arena *arena, u64 pos);
unimplemented void arena_clear(Arena *arena);

i64 copy_memory_block(void *dest, void *source, i64 byte_count);
i64 set_memory_block(void *dest, u8 val, i64 byte_count);
i64 move_memory_block(void *dest, void *source, i64 byte_count);
i64 compare_memory_block(void *a, void *b, i64 byte_count);

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
#define copy_string(dest, string)       copy_memory_block((dest), (string).str, (string).size)
#define copy_struct(dest, copy)         copy_memory_block(dest, copy, sizeof(*(copy)))
#define copy_array(dest, array, length) copy_memory_block(dest, array, sizeof(*(array) * length))

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
#define zero_memory_block(dest, byte_count) set_memory_block((void *) (dest), 0, (byte_count))
#define zero_struct(dest) zero_memory_block((dest), sizeof(*(dest)))
#define zero_literal(literal) zero_memory_block(literal, sizeof(literal))

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

void *arena_push(Arena *arena, u64 size)
{
  void *memory_given_back = NULL;

  size = align(size, arena->alignment);
  if ((arena->used + size) < arena->size)
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

i64 compare_memory_block(void *a, void *b, i64 byte_count)
{
  i64 result = 0;

  u8 *a_bytes = (u8 *) a;
  u8 *b_bytes = (u8 *) b;

  assert(a_bytes != NULL);
  assert(b_bytes != NULL);

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
#define compare_struct_shallow(a, b) compare_memory_block(a, b, sizeof(*a))

#define TRADER_MEMORY_H
#endif
