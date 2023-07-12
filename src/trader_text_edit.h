#if !defined(TRADER_TEXT_EDIT_H)
struct Text_Edit_Buffer
{
  Buffer          buf;
  u64             next_char_index;
  String_Encoding encoding;
};

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

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

#define TRADER_TEXT_EDIT_H
#endif
