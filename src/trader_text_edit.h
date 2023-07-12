#if !defined(TRADER_TEXT_EDIT_H)
struct Text_Edit_Buffer
{
  Buffer          buf;
  u64             next_char_index;
  String_Encoding encoding;
};

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i64 chars_to_delete);
internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb, i64 chars_to_delete);

// implementation
internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string)
{
  i64 chars_placed = -1;
  expect(teb->encoding == string_encoding_utf8);

  u64 one_past_string_last_char_pos = get_last_char_pos(string) + 1;
  if ((teb->next_char_index <= teb->buf.used) &&
      ((teb->buf.used + one_past_string_last_char_pos) <= teb->buf.size))
  {
    u8 *next_char_pos = teb->buf.data + teb->next_char_index;
    u64 bytes_to_move = teb->buf.used - teb->next_char_index; 

    move_memory_block(next_char_pos + one_past_string_last_char_pos,
                      next_char_pos,
                      bytes_to_move);
    copy_memory_block(next_char_pos,
                      string.str,
                      one_past_string_last_char_pos);
    teb->buf.used += one_past_string_last_char_pos;

    chars_placed = one_past_string_last_char_pos;
  }

  return(chars_placed);
}

internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string)
{
  i64 to_advance = text_edit_insert_string(teb, string);

  if (to_advance >= 0)
  {
    teb->next_char_index += to_advance;
  }

  return(to_advance);
}

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i64 chars_to_delete)
{
  if (teb->buf.used == 0) return(0);

  i64 bytes_deleted = -1;

  expect(teb->encoding == string_encoding_utf8);

  i64 final_pos = teb->next_char_index;
  while (chars_to_delete > 0)
  {
    i64 prev_pos = unicode_utf8_get_prev_char_pos(teb->buf.data, final_pos, teb->buf.used);
    if (prev_pos < 0)
    {
      final_pos = -1;
      break;
    }
    else
    {
      final_pos = prev_pos;
      chars_to_delete--;
    }
  }

  if (final_pos >= 0)
  {
    expect((i64) teb->next_char_index >= final_pos);
    i64 bytes_to_delete = teb->next_char_index - final_pos;

    move_memory_block(teb->buf.data + final_pos,
                      teb->buf.data + teb->next_char_index,
                      teb->buf.used - teb->next_char_index);

    teb->buf.used -= zero_memory_block((teb->buf.data + teb->buf.used) - bytes_to_delete, bytes_to_delete);
    teb->next_char_index = min(teb->next_char_index, teb->buf.used);

    bytes_deleted = bytes_to_delete;
  }

  return(bytes_deleted);
}

internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb, i64 chars_to_delete)
{
  i64 to_advance = text_edit_delete(teb, chars_to_delete);

  if (to_advance >= 0)
  {
    if (teb->next_char_index == teb->buf.used)
    {
      return(to_advance);
    }
    else
    {
      teb->next_char_index -= to_advance;
    }
  }

  return(to_advance);
}

#define TRADER_TEXT_EDIT_H
#endif
