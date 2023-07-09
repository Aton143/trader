#if !defined(TRADER_UNICODE_H)
internal inline i64 unicode_utf8_encode(u32 *code_points, u64 code_point_length, utf8 *put, u64 put_length);

// NOTE(antonio): implementation
internal inline i64 unicode_utf8_encode(u32 *code_points, u64 code_point_length, utf8 *put, u64 put_length)
{
  expect(code_points != NULL);
  expect(put        != NULL);

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

    if ((put_index + cur_length) < put_length)
    {
      put_index += cur_length;
      copy_memory_block(&put[put_index], temp_put, sizeof(temp_put[0]) * cur_length);
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

#define TRADER_UNICODE_H
#endif
