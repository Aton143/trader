#ifndef TRADER_UTILS_H

global u64 hash_seed = 0x400921fb54442d18;

#define buffer_from_string_literal_type(s) {(u8 *) (s), sizeof(s), sizeof(s)}

internal inline b32 is_in_buffer(Buffer *buf, i64 pos);
internal inline u32 count_set_bits(u64 bits);

internal u32 WELLRNG512(void);
internal void rng_init(void);
internal u32 rng_get_random32(void);
internal u64 rng_fill_buffer(u8 *buffer, u64 buffer_length);
internal u16 fletcher_sum(u8 *data, u32 count);
internal u64 hash_mix(u64 value);
internal u64 hash(u8 *buffer, u64 length);
internal u64 hash_c_string(char *str);
internal f32 abs(f32 a);
internal u64 difference_with_wrap(u64 a, u64 b);
internal b32 xorb(b32 a, b32 b);

#define TRADER_UTILS_H
#endif
