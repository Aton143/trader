#ifndef TRADER_MEMORY_H

#if OS_WINDOWS
#define alloca _alloca
#elif OS_LINUX
#include <alloca.h>
#endif

#include "trader_platform.h"
#include "trader_utils.h"

struct Bucket_List
{
  String_Const_utf8 tag;

  u8  *memory;
  u64  total_size;
  u32  bucket_max_size;

  u32  first_bucket;
  u32  next_available;
  u16  alignment;
  u16  header_size;
};

// NOTE(antonio): in-memory data
// header_size | size | header bytes | data ...
struct Bucket
{
  u32 next_bucket_id;
  u32 data_size;

  u8  data[1];
};

global_const u32 bucket_list_invalid_id = (u32) -1;

// TODO(antonio): store_ptr/load_ptr versions
// i.e. ring_buffer_append(..., &widget) caused issues in the past

global_const u32            thread_count                  = 2;
global       Thread_Context thread_contexts[thread_count] = {};

#if SHIP_MODE
global_const void *global_memory_start_addr = NULL;
#else
global_const void *global_memory_start_addr = (void *) tb(2);
#endif

global_const u64 global_memory_size     = mb(128);
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
internal inline Arena arena_make(void *start, u64 size, u64 alignment);
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
internal inline Ring_Buffer ring_buffer_make(Arena *arena,  u64 size);
internal inline Ring_Buffer ring_buffer_make(u8    *buffer, u64 size);
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

internal inline Bucket_List bucket_list_make(void              *memory,
                                             u64                total_size,
                                             u32                bucket_max_size,
                                             u16                header_size,
                                             u16                alignment,
                                             String_Const_utf8  tag);

internal inline Bucket_List bucket_list_make(Arena             *arena,
                                             u64                total_size,
                                             u32                bucket_max_size,
                                             u16                header_size,
                                             u16                alignment,
                                             String_Const_utf8  tag);


internal inline Bucket *bucket_list_get_from_id(Bucket_List *meta, u32 id);
internal inline Bucket *bucket_list_get_first(Bucket_List *meta);
internal inline Bucket *bucket_list_get_new_and_update(Bucket_List *meta);
internal inline void bucket_list_put_back(Bucket_List *meta, Bucket *put);

internal inline void *bucket_list_get_data_start(Bucket_List *meta, Bucket *bucket);
internal inline void *bucket_list_get_header_start(Bucket_List *meta, Bucket *bucket);

internal inline u32 bucket_list_get_count_fits_in_data(Bucket_List *meta, u32 size);

#define TRADER_MEMORY_H
#endif
