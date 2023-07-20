#if !defined(TRADER_TEXT_EDIT_H)

// NOTE(antonio): it may be better to make these data pointers and 
//                use characters in the API
struct Text_Range
{
  i64 start_index;
  i64 inclusive_end_index;
};

internal inline i64 range_get_length(Text_Range *range);

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

typedef u32 Text_Edit_Movement;
enum
{
  text_edit_movement_none,
  text_edit_movement_single,
  text_edit_movement_word,
  text_edit_movement_end,
};

internal inline Text_Edit_Buffer make_text_edit_buffer(Buffer          buf,
                                                       Text_Range      range    = {0, 0},
                                                       String_Encoding encoding = string_encoding_utf8);

internal inline utf8 *text_edit_get_start_ptr(Text_Edit_Buffer *teb);
internal inline utf8 *text_edit_get_end_ptr(Text_Edit_Buffer *teb);

internal inline i64 *text_edit_get_advancer_ptr(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection);
internal utf8 *text_edit_move_selection_step(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection = false);
internal void  text_edit_move_selection(Text_Edit_Buffer   *teb,
                                        i64                 dir,
                                        b32                 keep_selection = false,
                                        Text_Edit_Movement  movement_type  = text_edit_movement_none);

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i32 dir);

// implementation
internal inline i64 range_get_length(Text_Range *range)
{
  expect(range->start_index <= range->inclusive_end_index);
  i64 length = (range->inclusive_end_index - range->start_index);
  return(length);
}

internal inline Text_Edit_Buffer make_text_edit_buffer(Buffer buf, Text_Range range, String_Encoding encoding)
{
  Text_Edit_Buffer teb = {};

  teb.buf        = buf;
  teb.range      = range;
  teb.moving_end = NULL;
  teb.encoding   = encoding;

  expect(teb.encoding == string_encoding_utf8);

  return(teb);
}

internal inline utf8 *text_edit_get_start_ptr(Text_Edit_Buffer *teb)
{
  expect(teb->range.start_index >= 0);
  utf8 *start_ptr = &teb->buf.data[teb->range.start_index];
  return(start_ptr);
}

internal inline utf8 *text_edit_get_end_ptr(Text_Edit_Buffer *teb)
{
  utf8 *end_ptr = &teb->buf.data[teb->range.inclusive_end_index];
  return(end_ptr);
}

internal inline i64 *text_edit_get_advancer_ptr(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection)
{
  i64 *advancer_ptr = NULL;

  if (keep_selection)
  {
    if (dir > 0)
    {
      advancer_ptr = (teb->moving_end != NULL) ? teb->moving_end : &teb->range.inclusive_end_index;
    }
    else
    {
      advancer_ptr = (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;
    }
  }
  else
  {
    advancer_ptr = (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;;
  }

  return(advancer_ptr);
}

internal void  text_edit_move_selection(Text_Edit_Buffer   *teb,
                                        i64                 dir,
                                        b32                 keep_selection,
                                        Text_Edit_Movement  movement_type)
{
  expect(teb != NULL);

  i64 *advancer_ptr = text_edit_get_advancer_ptr(teb, dir, keep_selection);

  expect(dir != 0);
  expect(advancer_ptr != NULL);
  expect(teb->range.start_index <= teb->range.inclusive_end_index);

  i64 new_advancer_index = 0;
  i64 original_advancer  = *advancer_ptr;

  if (movement_type == text_edit_movement_single)
  {
    new_advancer_index = unicode_utf8_advance_char_pos(teb->buf.data, *advancer_ptr, teb->buf.used, (i32) dir);
  }
  else if (movement_type == text_edit_movement_word)
  {
    i64 cur_index      = unicode_utf8_advance_char_pos(teb->buf.data, *advancer_ptr, teb->buf.used, (i32) dir);
    i64 next_cur_index = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);

    b32 cur_at_delim = unicode_utf8_is_char_in_string(teb->buf.data + *advancer_ptr,     1, word_separators);
    b32 next_cur_at_delim = unicode_utf8_is_char_in_string(teb->buf.data + next_cur_index, 1, word_separators);

    if ((dir < 0) && (cur_at_delim || next_cur_at_delim))
    {
      if (cur_at_delim)
      {
        cur_index = next_cur_index;
      }

      while (is_between_exclusive(0, cur_index, (i64) teb->buf.used) &&
             unicode_utf8_is_char_in_string(teb->buf.data + cur_index, 1, word_separators))
      {
        cur_index = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);
      }
    }

    next_cur_index = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);

    while (is_between_exclusive(0, cur_index, (i64) teb->buf.used) &&
           !unicode_utf8_is_char_in_string(teb->buf.data + next_cur_index, 1, word_separators))
    {
      cur_index = next_cur_index;
      next_cur_index = unicode_utf8_advance_char_pos(teb->buf.data, next_cur_index, teb->buf.used, (i32) dir);
    }

    cur_at_delim      = unicode_utf8_is_char_in_string(teb->buf.data + *advancer_ptr, 1, word_separators);

    next_cur_index    = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);
    next_cur_at_delim = unicode_utf8_is_char_in_string(teb->buf.data + next_cur_index, 1, word_separators);

    if ((dir > 0) && (cur_at_delim || next_cur_at_delim))
    {
      if (cur_at_delim)
      {
        cur_index = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);
      }
      else
      {
        cur_index = next_cur_index;
      }

      while (is_between_exclusive(0, cur_index, (i64) teb->buf.used) &&
             unicode_utf8_is_char_in_string(teb->buf.data + cur_index, 1, word_separators))
      {
        cur_index = unicode_utf8_advance_char_pos(teb->buf.data, cur_index, teb->buf.used, (i32) dir);
      }
    }

    new_advancer_index = cur_index;
  }
  else if (movement_type == text_edit_movement_end)
  {
    if (dir > 0)
    {
      new_advancer_index = teb->buf.used;
    }
    else
    {
      new_advancer_index = 0;
    }
  }
  else
  {
    expect_message(false, "Should not get here");
    return;
  }

  *advancer_ptr = new_advancer_index;

  if (teb->range.start_index > teb->range.inclusive_end_index)
  {
    swap(i64, teb->range.start_index, teb->range.inclusive_end_index);
  }

  if (keep_selection)
  {
    if ((teb->moving_end == NULL) && (original_advancer != new_advancer_index))
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
    teb->range.start_index         = new_advancer_index;
    teb->range.inclusive_end_index = new_advancer_index;
    teb->moving_end                = NULL;
  }
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

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i32 dir)
{
  expect(dir != 0);

  i64 range_length = range_get_length(&teb->range);

  if ((teb->buf.used == 0) ||
      ((range_length == 0) &&
      (((dir < 0)  && (teb->range.start_index         == 0)) || 
       ((dir > 0)  && (teb->range.inclusive_end_index == (i64) teb->buf.used)))))
  {
    return(0);
  }

  b32 selection_active = (range_length >= 1);

  utf8 *start_ptr = NULL;
  utf8 *end_ptr   = NULL;

  if (range_length == 0)
  {
    if (dir < 0)
    {
      end_ptr   = text_edit_get_end_ptr(teb);
      start_ptr = teb->buf.data + unicode_utf8_advance_char_pos(teb->buf.data,
                                                                teb->range.start_index,
                                                                teb->buf.used, dir);
    }
    else
    {
      start_ptr = text_edit_get_start_ptr(teb);
      end_ptr   = teb->buf.data + unicode_utf8_advance_char_pos(teb->buf.data,
                                                                teb->range.inclusive_end_index,
                                                                teb->buf.used, dir);
    }

    expect(end_ptr >= start_ptr);
    range_length = end_ptr - start_ptr;
  }
  else
  {
    start_ptr = text_edit_get_start_ptr(teb);
  }

  utf8 *teb_end_ptr = teb->buf.data + teb->buf.used;

  i64 bytes_to_delete = unicode_utf8_encoding_length(start_ptr, range_length);
  i64 bytes_deleted   = -1;

  expect(teb->encoding == string_encoding_utf8);

  if (start_ptr != teb_end_ptr)
  {
    move_memory_block(start_ptr, end_ptr, teb->buf.used - teb->range.start_index);
  }

  teb->buf.used -= zero_memory_block((teb->buf.data + teb->buf.used) - bytes_to_delete, bytes_to_delete);
  // TODO(antonio): one char?
  if (selection_active)
  {
    bytes_deleted = 0;
  }
  else
  {
    bytes_deleted = bytes_to_delete - (dir > 0) ? range_length : 0;
  }

  teb->range.inclusive_end_index =
    (teb->range.start_index = clamp(0, teb->range.start_index - bytes_deleted, (i64) teb->buf.used));

  return(bytes_deleted);
}

#define TRADER_TEXT_EDIT_H
#endif
