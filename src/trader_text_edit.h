#if !defined(TRADER_TEXT_EDIT_H)

struct Text_Range
{
  i64 start_index;
  i64 inclusive_end_index;
};

struct Text_Edit_Buffer
{
  Buffer            buf;
  union
  {
    i64             next_char_index;
    Text_Range      range;
  };
  i64              *moving_end;
  String_Encoding   encoding;
};

internal Text_Edit_Buffer make_text_edit_buffer(Buffer          buf,
                                                Text_Range      range    = {0, 0},
                                                String_Encoding encoding = string_encoding_utf8);

// internal void text_edit_move_selection_forward(Text_Edit_Buffer *teb, i64 chars_to_advance, b32 keep_selection = false);
internal inline i64 *text_edit_get_advancer_ptr(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection);
internal utf8 *text_edit_move_selection_step(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection);
internal void  text_edit_move_selection(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection = false, b32 control = false);

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i64 chars_to_delete);
internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb, i64 chars_to_delete);

// implementation
internal Text_Edit_Buffer make_text_edit_buffer(Buffer buf, Text_Range range, String_Encoding encoding)
{
  Text_Edit_Buffer teb = {};

  teb.buf        = buf;
  teb.range      = range;
  teb.moving_end = NULL;
  teb.encoding   = encoding;

  expect(teb.encoding == string_encoding_utf8);

  return(teb);
}

internal inline i64 *text_edit_get_advancer_ptr(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection)
{
  i64 *where_to_put_advancer = NULL;

  if (keep_selection)
  {
    if (dir > 0)
    {
      where_to_put_advancer =  (teb->moving_end != NULL) ? teb->moving_end : &teb->range.inclusive_end_index;
    }
    else
    {
      where_to_put_advancer =  (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;
    }
  }
  else
  {
    where_to_put_advancer = (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;;
  }

  return(where_to_put_advancer);
}

internal void text_edit_move_selection(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection, b32 control)
{
  if (dir == 0)
  {
    return;
  }

  i64 *advancer_ptr;
  i64  possible_delim_index;

  if (control)
  {
    advancer_ptr         = text_edit_get_advancer_ptr(teb, dir, keep_selection);
    possible_delim_index = unicode_utf8_advance_char_pos(teb->buf.data, *advancer_ptr, teb->buf.used, (i32) dir);

    if (possible_delim_index == *advancer_ptr)
    {
      return;
    }

    b32 advancer_at_delim = unicode_utf8_is_char_in_string(&teb->buf.data[*advancer_ptr], 1, word_separators);
    b32 possible_at_delim = unicode_utf8_is_char_in_string(&teb->buf.data[possible_delim_index], 1, word_separators);

    if (advancer_at_delim || possible_at_delim)
    {
      if (advancer_at_delim)
      {
        text_edit_move_selection_step(teb, dir, keep_selection);
      }

      // TODO(antonio): needs to be the same one as last time
      while (unicode_utf8_is_char_in_string(text_edit_move_selection_step(teb, dir, keep_selection),
                                            1, word_separators));

      if (dir > 0)
      {
        return;
      }
    }
  }

  // NOTE(antonio): now at the "middle" of a "word"
  if (control)
  {
    advancer_ptr         = text_edit_get_advancer_ptr(teb, dir, keep_selection);
    possible_delim_index = unicode_utf8_advance_char_pos(teb->buf.data, *advancer_ptr, teb->buf.used, (i32) dir);

    i64 advance_encoding_index = *advancer_ptr;

    while (is_between_exclusive(0, advance_encoding_index, (i64) teb->buf.used) &&
           !unicode_utf8_is_char_in_string(teb->buf.data + possible_delim_index, 1, word_separators))
    {
      utf8 *advanced_encoding = text_edit_move_selection_step(teb, dir, keep_selection);
      advance_encoding_index  = (i64) (advanced_encoding - teb->buf.data );

      possible_delim_index = unicode_utf8_advance_char_pos(teb->buf.data,
                                                           (i64) advance_encoding_index,
                                                           teb->buf.used,
                                                           (i32) dir);
    }
  }
  else
  {
    text_edit_move_selection_step(teb, dir, keep_selection);
  }

  if (control && (dir > 0))
  {
    advancer_ptr         = text_edit_get_advancer_ptr(teb, dir, keep_selection);
    possible_delim_index = unicode_utf8_advance_char_pos(teb->buf.data, *advancer_ptr, teb->buf.used, (i32) dir);

    if (possible_delim_index == *advancer_ptr)
    {
      return;
    }

    b32 advancer_at_delim = unicode_utf8_is_char_in_string(&teb->buf.data[*advancer_ptr], 1, word_separators);
    b32 possible_at_delim = unicode_utf8_is_char_in_string(&teb->buf.data[possible_delim_index], 1, word_separators);

    if (advancer_at_delim || possible_at_delim)
    {
      if (advancer_at_delim)
      {
        text_edit_move_selection_step(teb, dir, keep_selection);
      }

      while (unicode_utf8_is_char_in_string(text_edit_move_selection_step(teb, dir, keep_selection),
                                            1, word_separators));
    }
  }
}

internal utf8 *text_edit_move_selection_step(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection)
{
  expect(teb != NULL);

  i64 *advancer_ptr = text_edit_get_advancer_ptr(teb, dir, keep_selection);

  expect(advancer_ptr != NULL);
  expect(teb->range.start_index <= teb->range.inclusive_end_index);

  i64 advancer          = *advancer_ptr;
  i64 original_advancer = advancer;

  // TODO(antonio): replace with get advance char pos
  while (is_in_buffer(&teb->buf, advancer) && !unicode_utf8_is_start(teb->buf.data[advancer]))
  {
    advancer += dir;
  }

  if (is_in_buffer(&teb->buf, advancer + dir))
  {
    advancer += dir;
  }

  *advancer_ptr = advancer;

  if (keep_selection)
  {
    if ((teb->moving_end == NULL) && (original_advancer != advancer))
    {
      teb->moving_end = (dir > 0) ? &teb->range.inclusive_end_index : &teb->range.start_index;
    }

    if (teb->range.start_index == teb->range.inclusive_end_index)
    {
      teb->moving_end = NULL;
    }
  }
  else
  {
    teb->range.start_index         = advancer;
    teb->range.inclusive_end_index = advancer;
    teb->moving_end                = NULL;
  }

  expect(teb->range.start_index <= teb->range.inclusive_end_index);

  utf8 *encoding_at_advancer = teb->buf.data + advancer;
  return(encoding_at_advancer);
}

// TODO(antonio): deletion must occur if range difference > 0 
internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string)
{
  i64 chars_placed = -1;
  expect(teb->encoding == string_encoding_utf8);

  u64 one_past_string_last_char_pos = get_last_char_pos(string) + 1;
  if ((teb->next_char_index <= (i64) teb->buf.used) &&
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
    teb->range.start_index         += to_advance;
    teb->range.inclusive_end_index  = teb->range.start_index;
  }

  return(to_advance);
}

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i64 chars_to_delete)
{
  if ((teb->buf.used == 0) || (teb->next_char_index == 0)) return(0);

  i64 bytes_deleted = -1;

  expect(teb->encoding == string_encoding_utf8);

  i64 final_pos = teb->next_char_index;
  while (chars_to_delete > 0)
  {
    i64 prev_pos = unicode_utf8_get_prev_char_pos(teb->buf.data, (i64) final_pos, (i64) teb->buf.used);
    if (prev_pos < 0)
    {
      final_pos = 0;
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

    bytes_deleted = bytes_to_delete;
  }

  return(bytes_deleted);
}

internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb, i64 chars_to_delete)
{
  i64 to_advance = text_edit_delete(teb, chars_to_delete);

  if (to_advance > 0)
  {
    text_edit_move_selection(teb, -to_advance);
  }

  teb->next_char_index = clamp(0, teb->next_char_index, (i64) teb->buf.used);

  return(to_advance);
}

#define TRADER_TEXT_EDIT_H
#endif
