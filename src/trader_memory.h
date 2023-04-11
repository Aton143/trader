#ifndef TRADER_MEMORY_H

struct Arena
{
  Arena *next;
  Arena *prev;

  void *start;
  u64   used;
  u64   size;
  u64   alignment;
};

Arena *arena_alloc(void);
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

#define copy_struct(dest, copy) copy_memory_block(dest, sizeof(*(copy)))

i64 set_memory_block(void *dest, u8 val, i64 byte_count)
{
  u8 *dest_bytes = (u8 *) dest;

  i64 byte_index;
  for (byte_index = 0;
       byte_index < byte_count;
       ++byte_count)
  {
    dest_bytes[byte_index] = val;
  }

  return(byte_index);
}

#define zero_memory_block(dest, byte_count) set_memory_block((dest), 0, (byte_count))
#define zero_struct(dest) zero_memory_block((dest), sizeof(*(dest)))

Arena *arena_alloc(void)
{
  Arena *new_arena = NULL;

  if (global_memory_pool.arena_free_list_head != NULL)
  {
    Arena *head = NULL;
    Arena *tail = NULL;

    new_arena = global_memory_pool.arena_free_list_head;

    if (global_memory_pool.arena_list_head != NULL)
    {
      head = global_memory_pool.arena_list_head;
      tail = head->prev;

      new_arena->next = head;
      new_arena->prev = tail;
    }
    else
    {
      global_memory_pool.arena_list_head = new_arena;

      head = new_arena;
      tail = new_arena;
    }

    tail->next = new_arena;
    head->prev = new_arena;

    {
      Arena *old_head = global_memory_pool.arena_free_list_head;
      Arena *tent_head = old_head->next;

      assert_message_always("was not expecting this!");

      if (old_head == tent_head)
      {
        global_memory_pool.arena_free_list_head = NULL;
      }
      else
      {
        global_memory_pool.arena_free_list_head = tent_head;
      }
    }
  }
  // TODO(antonio): what if we run out of arenas?
  else
  {
    assert_message_always("was not expecting you to run out of arenas");
  }

  return(new_arena);
}

void arena_release(Arena *arena)
{
  zero_struct(arena);

  arena->next = global_memory_pool.arena_free_list_head;
  global_memory_pool.arena_free_list_head = arena;
}

#define TRADER_MEMORY_H
#endif
