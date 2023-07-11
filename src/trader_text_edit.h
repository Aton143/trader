#if !defined(TRADER_TEXT_EDIT_H)
struct Text_Edit_Buffer
{
  Buffer          text;
  u64             next_char_index;
  String_Encoding encoding;
};

internal void text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string);


// implementation
internal void text_edit_insert_string(Text_Edit_Buffer *teb, String_utf8 string)
{
  unused(teb);
  unused(string);

  expect(teb->encoding == string_encoding_utf8);

  // u64 string_last_char_pos;
  //if (teb->text.used + string.size
}

#define TRADER_TEXT_EDIT_H
#endif
