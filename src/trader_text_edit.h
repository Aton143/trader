#if !defined(TRADER_TEXT_EDIT_H)
struct Text_Edit_Buffer
{
  Buffer          text;
  u64             next_char_index;
  String_Encoding encoding;
};

#define TRADER_TEXT_EDIT_H
#endif
