#include "trader_utils.h"

internal inline b32 is_in_buffer(Buffer *buf, i64 pos)
{
  b32 is_in = (0 <= pos) && (pos <= ((i64) buf->used));
  return(is_in);
}

internal inline u32 count_set_bits(u64 bits)
{
    u32 count = 0;
    while (bits != 0)
    {
        bits = bits & (bits - 1);
        count++;
    }
    return count;
}

// WELL512 rng, Chris Lomont, www.lomont.org

// initialize state to random bits
global u32 rng_state[16] = 
{
  0b10000101000010000101010101110010,
  0b10111110011111001111100010010110,
  0b11111100000010010011100000111001,
  0b11011000000001011100001000000101,
  0b10011100011101111001101101111101,
  0b11111010001101010001011101010011,
  0b10101001110010011111011111000001,
  0b01000011001011000000101100110111,
  0b01010111010101110110001110101010,
  0b11011011010111000110100010100100,
  0b11001010111101010000101010110000,
  0b00011001101000010011101011100000,
  0b01111010111010100110000001111111,
  0b01000100101010010010101100111100,
  0b00101000001010110011011011101101,
  0b00001111100000101110011010101011,
};

// init should also reset this to 0
global u32 rng_index = 0;

// return 32 bit random number
internal u32 WELLRNG512(void)
{
  u32 a, b, c, d;
  a = rng_state[rng_index];
  c = rng_state[(rng_index + 13) & 15];
  b = a ^ c ^ (a << 16) ^ (c << 15);
  c = rng_state[(rng_index + 9) & 15];
  c ^= (c >> 11);
  a = rng_state[rng_index] = b ^ c;
  d = a^((a << 5) & 0xDA442D24UL);
  rng_index = (rng_index + 15) & 15;
  a = rng_state[rng_index];
  rng_state[rng_index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
  return rng_state[rng_index];
}

internal void rng_init(void)
{
  rng_index = 0;
}

internal u32 rng_get_random32(void)
{
  u32 result = WELLRNG512();
  return(result);
}

internal f32 rng_get_random_01_f32(void)
{
  u32 random_u16 = rng_get_random32() >> 16;
  f32 result = ((f32) random_u16) / ((f32) max_u16);
  return(result);
}

internal f32 rng_get_random_between_f32(f32 min, f32 max)
{
  expect(min <= max);
  f32 result = lerpf(min, rng_get_random_01_f32(), max);
  return(result);
}

internal u64 rng_fill_buffer(u8 *buffer, u64 buffer_length)
{
  u32 *buffer32 = (u32 *) buffer;
  u64 length32 = buffer_length / sizeof(*buffer32);

  for (u64 index32 = 0;
       index32 < length32;
       ++index32)
  {
    buffer32[index32] = rng_get_random32();
  }

  u64 remaining = buffer_length % sizeof(*buffer32);

  buffer += length32 * sizeof(*buffer32);
  for (u64 remaining_index = 0;
       remaining_index < remaining;
       ++remaining_index)
  {
    buffer[remaining_index] = (u8) rng_get_random32();
  }

  return(buffer_length);
}

internal u16 fletcher_sum(u8 *data, u32 count)
{
   u8 sum1 = 0;
   u8 sum2 = 0;

   for (u32 index = 0;
        index < count;
        ++index)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }

   return((sum2 << 8) | sum1);
}

internal u64 hash_mix(u64 value)
{
 value ^= value >> 23;
 value *= 0x2127599bf4325c37ULL;
 value ^= (value) >> 47;
 return(value);
}

internal u64 hash(u8 *buffer, u64 length)
{
  u64 multiplier = 0x880355f21e6d1965ULL;
  u64 *buffer_position = (u64 *) buffer;
  u64 *buffer_end = buffer_position + (length / 8);

  u8 *remaining_buffer_position;
  u64 result = hash_seed ^ (length * multiplier);

  u64 current_buffer_value;
  while (buffer_position != buffer_end) {
    current_buffer_value = *buffer_position;
    buffer_position++;
    result ^= hash_mix(current_buffer_value);
    result *= multiplier;
  }

  remaining_buffer_position = (u8 *) buffer_position;
  current_buffer_value = 0;

  switch (length & 7) {
  case 7: current_buffer_value ^= ((u64) remaining_buffer_position[6]) << 48;
  case 6: current_buffer_value ^= ((u64) remaining_buffer_position[5]) << 40;
  case 5: current_buffer_value ^= ((u64) remaining_buffer_position[4]) << 32;
  case 4: current_buffer_value ^= ((u64) remaining_buffer_position[3]) << 24;
  case 3: current_buffer_value ^= ((u64) remaining_buffer_position[2]) << 16;
  case 2: current_buffer_value ^= ((u64) remaining_buffer_position[1]) << 8;
  case 1: current_buffer_value ^= ((u64) remaining_buffer_position[0]);
          result ^= hash_mix(current_buffer_value);
          result *= multiplier;
  }

  return(hash_mix(result));
}

internal u64 hash_c_string(char *str)
{
  return(hash((u8 *) str, c_string_length(str)));
}

// TODO(antonio): this of course doesn't really make sense
internal u64 difference_with_wrap(u64 a, u64 b)
{
  u64 b_a_diff;

  if (a >= b)
  {
    b_a_diff = a - b;
  }
  else
  {
    b_a_diff = (max_u64 - b) + a;
  }

  return(b_a_diff);
}

internal b32 xorb(b32 a, b32 b)
{
  b32 res = (!!a) ^ (!!b); 
  return(res);
}

