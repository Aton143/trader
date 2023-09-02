#ifndef TRADER_UTILS_H

global u64 hash_seed = 0x400921fb54442d18;

#define buffer_from_string_literal_type(s) {(u8 *) (s), sizeof(s), sizeof(s)}

internal inline b32 is_in_buffer(Buffer *buf, i64 pos);
internal inline u32 count_set_bits(u64 bits);

internal u32 WELLRNG512(void);
internal void rng_init(void);
internal u32 rng_get_random32(void);
internal u64 rng_fill_buffer(u8 *buffer, u64 buffer_length);
internal inline f32 rng_get_random_01_f32(void);
internal inline f32 rng_get_random_between_f32(f32 min, f32 max);
internal inline u32 rng_get_random_between_end_exclusive_u32(u32 min, u32 max);
#define rng_get_random_rgba_f32(end_alpha) \
  rgba(rng_get_random_01_f32(), rng_get_random_01_f32(), rng_get_random_01_f32(), end_alpha)

internal u16 fletcher_sum(u8 *data, u32 count);
internal u64 hash_mix(u64 value);
internal u64 hash(u8 *buffer, u64 length);
internal u64 hash_c_string(char *str);
internal f32 abs(f32 a);
internal u64 difference_with_wrap(u64 a, u64 b);
internal b32 xorb(b32 a, b32 b);

internal inline u16 popcount16(u16 n);
internal inline u32 popcount32(u32 n);
internal inline u64 popcount64(u64 n);

internal inline u16 byte_swap16(u16 val);
internal inline u32 byte_swap32(u32 val);
internal inline u64 byte_swap64(u64 val);

#define TRADER_UTILS_H
#endif
