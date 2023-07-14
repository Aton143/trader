#if !defined(TRADER_UNICODE_H)

global_const String_utf8 word_separators = str_from_lit("`~!@#$%^&*()-=+[{]}\\|;:'\",.<>/?", utf8);

// TODO(antonio): may be worth using the same length idea for all of these
// TODO(antonio): error-checking
internal inline b32 unicode_utf8_is_start(utf8 encoding_char);

// NOTE(antonio): can stay in the same pos
internal inline i64 unicode_utf8_get_next_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

// NOTE(antonio): will move forward (if in bounds)
internal inline i64 unicode_utf8_advance_char_pos(utf8 *start, i64 start_pos, i64 encoding_size_in_bytes, i32 dir);

internal inline i64 unicode_utf8_get_next_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_advance_to_word(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes, i32 dir);
internal inline i64 unicode_utf8_get_next_word_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_word_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_encoding_length(utf8 *encoding);
internal inline i64 unicode_utf8_encode(u32 *code_points, i64 code_point_length, utf8 *put, i64 put_pos, i64 put_length);

internal inline i64 unicode_utf8_is_char_in_string(utf8 *encoding, i32 encoding_length, String_utf8 string);

// NOTE(antonio): implementation
internal inline i64 unicode_utf8_encode(u32 *code_points, i64 code_point_length, utf8 *put, i64 put_pos, i64 put_length)
{
  expect(code_points != NULL);
  expect(put         != NULL);

  i64 res         = 0;
  u8  temp_put[4] = {};
  i64 put_index   = 0;

  for (i64 code_point_index = 0;
       code_point_index < code_point_length;
       ++code_point_index)
  {
    u32 cur_code_point = code_points[code_point_index];
    i32 cur_length = 0;

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

internal inline i64 unicode_utf8_get_next_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes)
{
  i64 next_char_pos = unicode_utf8_advance_char_pos(encoding_start, encoding_pos, encoding_size_in_bytes, 1);
  return(next_char_pos);
}

internal inline i64 unicode_utf8_get_prev_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes)
{
  i64 prev_char_pos = unicode_utf8_advance_char_pos(encoding_start, encoding_pos, encoding_size_in_bytes, 1);
  return(prev_char_pos);
}

internal inline i64 unicode_utf8_get_next_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes)
{
  i64 next_start_char_pos = -1;

  if (encoding_pos <= encoding_size_in_bytes)
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

internal inline i64 unicode_utf8_get_prev_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes)
{
  i64 prev_start_char_pos = -1;

  if (encoding_pos <= encoding_size_in_bytes)
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

internal inline i64 unicode_utf8_advance_char_pos(utf8 *encoding_start,
                                                  i64   encoding_pos,
                                                  i64   encoding_size_in_bytes,
                                                  i32   dir)
{
  i64 new_char_pos = encoding_pos + dir;

  while ((0 <= new_char_pos) && (new_char_pos < encoding_size_in_bytes) && 
         !unicode_utf8_is_start(encoding_start[new_char_pos]))
  {
    new_char_pos += dir;
  }

  new_char_pos = clamp(0, new_char_pos, encoding_size_in_bytes);
  return(new_char_pos);
}

internal inline i64 unicode_utf8_advance_to_word(utf8 *encoding_start,
                                                 i64   encoding_pos,
                                                 i64   encoding_size_in_bytes,
                                                 i32   dir)
{
  i64 new_word_pos = encoding_pos;

  do
  {
    new_word_pos = unicode_utf8_advance_char_pos(encoding_start, new_word_pos, encoding_size_in_bytes, dir);
  } while (!unicode_utf8_is_char_in_string(&encoding_start[new_word_pos],
                                           (i32) unicode_utf8_encoding_length(&encoding_start[new_word_pos]),
                                           word_separators));

  return(new_word_pos);
}

internal inline i64 unicode_utf8_is_char_in_string(utf8 *encoding, i32 encoding_length, String_utf8 string)
{
  expect(string.str != NULL);

  i64 result = -1;

  for (i64 string_index = 0;
       string_index < ((i64) string.size) - encoding_length; 
       ++string_index)
  {
    if (compare_memory_block(encoding, string.str, encoding_length))
    {
      result = (i64) string_index;
      break;
    }
  }

  return(result);
}

internal inline i64 unicode_utf8_encoding_length(utf8 *encoding)
{
  i64 res = unicode_utf8_get_next_char_pos(encoding, 0, 5);
  return(res);
}

#define TRADER_UNICODE_H
#endif
