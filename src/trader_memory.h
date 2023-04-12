#ifndef TRADER_MEMORY_H

struct Arena
{
  u8   *start;
  u64   size;
  u64   used;
  u64   alignment;
};

Arena *arena_alloc(u64 size, u64 alignment);
void arena_release(Arena *arena);

void *arena_push(Arena *arena, u64 size);
void *arena_push_zero(Arena *arena, u64 size);

#define push_array(arena, type, count) (type *) arena_push((arena), sizeof(type)*(count))
#define push_array_zero(arena, type, count) (type *) arena_push_zero((arena), sizeof(type)*(count))
#define push_struct(arena, type) push_array((arena), (type), 1)
#define push_struct_zero(arena, type) push_array_zero((arena), (type), 1)

void arena_pop(Arena *arena, u64 size);

u64 arena_get_pos(Arena *arena);

void arena_set_pos_back(Arena *arena, u64 pos);
void arena_clear(Arena *arena);

struct Memory_Pool
{
  u8 *start;
  u64 size;
  u64 used;

  Arena *arena_free_list_head;
  Arena *arena_list_head;
};

static Arena global_arena_list[128]   = {};
static Memory_Pool global_memory_pool = {};

i64 copy_memory_block(void *dest, void *source, i64 byte_count);
i64 set_memory_block(void *dest, u8 val, i64 byte_count);

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
#define copy_string(dest, string) copy_memory_block((dest), (string).str, (string).size)
#define copy_struct(dest, copy) copy_memory_block(dest, sizeof(*(copy)))

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

#define zero_memory_block(dest, byte_count) set_memory_block((dest), 0, (byte_count))
#define zero_struct(dest) zero_memory_block((dest), sizeof(*(dest)))

void *arena_push(Arena *arena, u64 size)
{
  void *memory_given_back = NULL;

  size = align(size, arena->alignment);
  if ((arena->used + size) < arena->size)
  {
    memory_given_back = (void *) (arena->start + arena->used);
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

#define TRADER_MEMORY_H
#endif
