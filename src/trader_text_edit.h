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

internal void text_edit_move_selection(Text_Edit_Buffer   *teb,
                                       i64                 dir,
                                       b32                 keep_selection = false,
                                       Text_Edit_Movement  movement_type  = text_edit_movement_none);

internal i64 text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);
internal i64 text_edit_insert_string_and_advance(Text_Edit_Buffer *teb, String_utf8 string);

internal i64 text_edit_delete(Text_Edit_Buffer *teb, i32 dir);

#define TRADER_TEXT_EDIT_H
#endif
