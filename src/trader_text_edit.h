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

internal inline Text_Edit_Buffer make_text_edit_buffer(Buffer          buf,
                                                       Text_Range      range    = {0, 0},
                                                       String_Encoding encoding = string_encoding_utf8);

internal inline utf8 *text_edit_get_start_ptr(Text_Edit_Buffer *teb);
internal inline utf8 *text_edit_get_end_ptr(Text_Edit_Buffer *teb);

internal inline i64 *text_edit_get_advancer_ptr(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection);
internal utf8 *text_edit_move_selection_step(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection = false);
internal void  text_edit_move_selection(Text_Edit_Buffer *teb, i64 dir, b32 keep_selection = false, b32 control = false);

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i64 chars_to_delete);
internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb, i64 chars_to_delete);

// implementation
internal inline i64 range_get_length(Text_Range *range)
{
  expect(range->start_index <= range->inclusive_end_index);
  i64 length = (range->inclusive_end_index - range->start_index) + 1;
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
      advancer_ptr =  (teb->moving_end != NULL) ? teb->moving_end : &teb->range.inclusive_end_index;
    }
    else
    {
      advancer_ptr =  (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;
    }
  }
  else
  {
    advancer_ptr = (teb->moving_end != NULL) ? teb->moving_end : &teb->range.start_index;;
  }

  return(advancer_ptr);
}

// TODO(antonio): could be better to do motion and then swap the start and end indices
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

    i64 advance_encoding_index;//  = *advancer_ptr;

    do
    {
      utf8 *advanced_encoding = text_edit_move_selection_step(teb, dir, keep_selection);
      advance_encoding_index  = (i64) (advanced_encoding - teb->buf.data );

      possible_delim_index = unicode_utf8_advance_char_pos(teb->buf.data,
                                                           (i64) advance_encoding_index,
                                                           teb->buf.used,
                                                           (i32) dir);
    }
    while (is_between_exclusive(0, advance_encoding_index, (i64) teb->buf.used) &&
           !unicode_utf8_is_char_in_string(teb->buf.data + possible_delim_index, 1, word_separators));
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

// TODO(antonio): weirdness at the buffer end
internal i64 text_edit_delete(Text_Edit_Buffer *teb)
{
  if ((teb->buf.used == 0) ||
      ((teb->range.start_index == teb->range.inclusive_end_index) && 
       (teb->range.start_index == 0)))
  {
    return(0);
  }

  utf8 *start_ptr   =
    (teb->range.start_index == 0) ?
    text_edit_get_start_ptr(teb) :
    teb->buf.data + unicode_utf8_get_prev_char_pos(teb->buf.data, teb->range.start_index, teb->buf.used);

  utf8 *end_ptr     = text_edit_get_end_ptr(teb);
  utf8 *teb_end_ptr = teb->buf.data + teb->buf.used;

  i64 bytes_to_delete = unicode_utf8_encoding_length(start_ptr, range_get_length(&teb->range));
  i64 bytes_deleted   = -1;

  expect(teb->encoding == string_encoding_utf8);

  if (start_ptr != teb_end_ptr)
  {
    move_memory_block(start_ptr, end_ptr, teb->buf.used - teb->range.start_index);
  }

  teb->buf.used -= zero_memory_block((teb->buf.data + teb->buf.used) - bytes_to_delete, bytes_to_delete);
  // TODO(antonio): one char?
  bytes_deleted = bytes_to_delete - ((end_ptr == teb_end_ptr) && (bytes_to_delete > 1)) ? 1 : 0;

  return(bytes_deleted);
}

internal i64 text_edit_delete_and_advance(Text_Edit_Buffer *teb)
{
  i64 to_advance = text_edit_delete(teb);

  while (to_advance > 0)
  {
    text_edit_move_selection_step(teb, -1);
    to_advance--;
  }

  teb->next_char_index = clamp(0, teb->next_char_index, (i64) teb->buf.used);

  return(to_advance);
}

#define TRADER_TEXT_EDIT_H
#endif
