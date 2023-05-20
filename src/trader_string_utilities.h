#ifndef TRADER_STRING_H
#include <wchar.h>

internal b32 is_newline(char c);

internal b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b);

internal u64 c_string_length(char *string);
internal u64 c_string_length(char *string, u64 max_length);

internal u64 c_string_length(utf8 *string);
internal u64 c_string_length(utf8 *string, u64 max_length);

internal u64 c_string_length(utf16 *string);
internal u64 c_string_count(utf16 *string);

internal String_Const_char string_const_char(char *string);
internal String_Const_utf16 string_const_utf16(utf16 *string);
internal u64 base64_encode(u8 *out_buffer, u8 *data, u64 data_length);

// Implementation
internal b32 is_newline(char c)
{
  b32 result = ((c == '\r') || 
                (c == '\n'));
  return(result);
}

internal b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b)
{
  b32 result = true;

  if (a.size == b.size)
  {
    for (u64 index = 0;
         index < a.size;
         ++index)
    {
      if (a.str[index] != b.str[index])
      {
        result = false;
        break;
      }
    }
  }
  else
  {
    result = false;
  }

  return(result);
}

internal u64 c_string_length(char *string)
{
  u64 result = 0;
  while (string[result] != '\0')
  {
    result++;
  }
  return(result);
}

internal u64 c_string_length(char *string, u64 n)
{
  u64 result = 0;
  while ((n-- > 0) && string[result] != '\0')
  {
    result++;
  }
  return(result);
}

internal u64 c_string_length(utf8 *string)
{
  return(c_string_length((char *) string));
}

internal u64 c_string_length(utf8 *string, u64 max_length)
{
  return(c_string_length((char *) string, max_length));
}

internal u64 c_string_length(utf16 *string)
{
  u64 result = 0;
  while (string[result] != '\0') {
    result++;
  }
  return(result);
}

internal u64 c_string_count(utf16 *string)
{
  u64 result = wcslen(string); 
  return(result);
}

internal String_Const_char string_const_char(char *string)
{
  String_Const_char result = {};

  result.str = string;
  result.size = c_string_length(string);

  return(result);
}

internal String_Const_utf16 string_const_utf16(utf16 *string)
{
  String_Const_utf16 result = {};

  result.str = string;
  result.size = c_string_length(string);

  return(result);
}

global_const u8 base64_encoding_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

global_const i32 base64_mod_table[] = {0, 2, 1};

// from https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
internal u64 base64_encode(u8 *out_buffer,
                           u8 *data,
                           u64 data_length)
{
  u64 result = 4 * ((data_length + 2) / 3);

  for (i32 i = 0, j = 0;
       i < data_length;
       )
  {
    u32 octet_a = i < data_length ? (u8) data[i++] : 0;
    u32 octet_b = i < data_length ? (u8) data[i++] : 0;
    u32 octet_c = i < data_length ? (u8) data[i++] : 0;

    u32 triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    out_buffer[j++] = base64_encoding_table[(triple >> 3 * 6) & 0x3F];
    out_buffer[j++] = base64_encoding_table[(triple >> 2 * 6) & 0x3F];
    out_buffer[j++] = base64_encoding_table[(triple >> 1 * 6) & 0x3F];
    out_buffer[j++] = base64_encoding_table[(triple >> 0 * 6) & 0x3F];
  }

  for (i32 i = 0;
       i < base64_mod_table[data_length % 3];
       i++)
  {
    out_buffer[result - 1 - i] = '=';
  }

  return(result);
}

#define TRADER_STRING_H
#endif
