#ifndef TRADER_STRING_H
#include <wchar.h>

u64 c_string_length(char *string);
u64 c_string_length(utf16 *string);

String_Const_char string_const_char(char *string);
String_Const_utf16 string_const_utf16(utf16 *string);

// Implementation
u64 c_string_length(char *string)
{
  u64 result = 0;
  while (string[result] != '\0') {
    result++;
  }
  return(result);
}

u64 c_string_length(utf16 *string)
{
  u64 result = 0;
  while (string[result] != '\0') {
    result++;
  }
  return(result);
}

String_Const_char string_const_char(char *string)
{
  String_Const_char result = {};

  result.str = string;
  result.size = c_string_length(string);

  return(result);
}

String_Const_utf16 string_const_utf16(utf16 *string)
{
  String_Const_utf16 result = {};

  result.str = string;
  result.size = c_string_length(string);

  return(result);
}

#define TRADER_STRING_H
#endif
