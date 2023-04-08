#ifndef TRADER_STRING_H
#include <wchar.h>

u64 get_length_c_string_char(char *string);
u64 get_length_c_string_utf16(utf16 *string);

// Implementation
u64 get_length_c_string_char(char *string)
{
  i64 result = 0;
  while (string[result] != '\0') {
    result++;
  }
  return(result);
}

u64 get_length_c_string_utf16(utf16 *string)
{
  u64 result = (u64) wcslen((const wchar_t *) string);
  return(result);
}

#define TRADER_STRING_H
#endif
