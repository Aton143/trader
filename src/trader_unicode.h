#if !defined(TRADER_UNICODE_H)
internal inline b32 unicode_utf8_is_start(utf8 encoding_char);

internal inline i64 unicode_utf8_get_next_start_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_start_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_get_next_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_encode(u32 *code_points, u64 code_point_length, utf8 *put, u64 put_pos, u64 put_length);

// NOTE(antonio): implementation
internal inline i64 unicode_utf8_encode(u32 *code_points, u64 code_point_length, utf8 *put, u64 put_pos, u64 put_length)
{
  expect(code_points != NULL);
  expect(put         != NULL);

  i64 res         = 0;
  u8  temp_put[4] = {};
  u64 put_index   = 0;

  for (u64 code_point_index = 0;
       code_point_index < code_point_length;
       ++code_point_index)
  {
    u32 cur_code_point = code_points[code_point_index];
    u32 cur_length = 0;

    zero_array(temp_put, u8, 4);
    if (cur_code_point <= 0x7f)
    {
      cur_length++;
      temp_put[0] = cur_code_point & 0x7f;

      cur_length = 1;
    }
    else if ((0x80 <= cur_code_point) && (cur_code_point <= 0x7ff))
    {
      temp_put[1] = (cur_code_point & 0x7f);

      cur_code_point >>= 7;
      temp_put[0] = 0b11000000 | (cur_code_point & 0x1f);

      cur_length = 2;
    }
    else if ((0x800 <= cur_code_point) && (cur_code_point <= 0xffff))
    {
      temp_put[2] = (cur_code_point & 0x7f);

      cur_code_point >>= 7;
      temp_put[1] = 0b11000000 | (cur_code_point & 0x1f);

      cur_code_point >>= 5;
      temp_put[0] = 0b11100000 | (cur_code_point & 0xf);

      cur_length = 3;
    }
    else if ((0x10000 <= cur_code_point) && (cur_code_point <= 0x10ffff))
    {
      temp_put[3] = (cur_code_point & 0x7f);

      cur_code_point >>= 7;
      temp_put[2] = 0b11000000 | (cur_code_point & 0x1f);

      cur_code_point >>= 5;
      temp_put[1] = 0b11100000 | (cur_code_point & 0xf);

      cur_code_point >>= 4;
      temp_put[0] = 0b11110000 | (cur_code_point & 0x7);

      cur_length = 4;
    }

    if ((put_pos + put_index + cur_length) < put_length)
    {
      copy_memory_block(&put[put_pos + put_index], temp_put, sizeof(temp_put[0]) * cur_length);
      put_index += cur_length;
      res++;
    }
    else
    {
      res = -res - 1;
      break;
    }
  }

  return(res);
}

internal inline b32 unicode_utf8_is_start(utf8 encoding_char)
{
  b32 one_byte_start   = ((encoding_char >> 7) == 0b0);
  b32 two_byte_start   = ((encoding_char >> 5) == 0b110);
  b32 three_byte_start = ((encoding_char >> 4) == 0b1110);
  b32 four_byte_start  = ((encoding_char >> 3) == 0b11110);

  b32 is_start = (one_byte_start || two_byte_start) || (three_byte_start || four_byte_start);
  return(is_start);
}

internal inline i64 unicode_utf8_get_next_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes)
{
  i64 next_char_pos = 0;

  while (((encoding_pos + next_char_pos) < encoding_size_in_bytes) &&
         !unicode_utf8_is_start(encoding_start[encoding_pos + next_char_pos]))
  {
    next_char_pos++;
  }

  if ((encoding_pos + next_char_pos) >= encoding_size_in_bytes)
  {
    return(-1);
  }

  return(encoding_pos + next_char_pos);
}

internal inline i64 unicode_utf8_get_prev_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes)
{
  i64 prev_char_pos = 0;

  if (encoding_pos >= encoding_size_in_bytes)
  {
    return (-1);
  }

  while (((encoding_pos - prev_char_pos) >= 0) &&
         !unicode_utf8_is_start(encoding_start[encoding_pos - prev_char_pos]))
  {
    prev_char_pos++;
  }

  if ((encoding_pos + prev_char_pos) < 0)
  {
    return(-1);
  }

  return(encoding_pos - prev_char_pos);
}

internal inline i64 unicode_utf8_get_next_start_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes)
{
  i64 next_start_char_pos = -1;

  if (encoding_pos < encoding_size_in_bytes)
  {
    if (unicode_utf8_is_start(encoding_start[encoding_pos]))
    {
      next_start_char_pos = encoding_pos;
    }
    else
    {
      next_start_char_pos = unicode_utf8_get_next_char_pos(encoding_start, encoding_pos, encoding_size_in_bytes);
    }
  }

  return(next_start_char_pos);
}

internal inline i64 unicode_utf8_get_prev_start_char_pos(utf8 *encoding_start, u64 encoding_pos, u64 encoding_size_in_bytes)
{
  i64 prev_start_char_pos = -1;

  if (encoding_pos < encoding_size_in_bytes)
  {
    if (unicode_utf8_is_start(encoding_start[encoding_pos]))
    {
      prev_start_char_pos = encoding_pos;
    }
    else
    {
      prev_start_char_pos = unicode_utf8_get_prev_char_pos(encoding_start, encoding_pos, encoding_size_in_bytes);
    }
  }

  return(prev_start_char_pos);
}

#define TRADER_UNICODE_H
#endif
