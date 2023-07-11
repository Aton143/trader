#ifndef TRADER_STRING_H
#include <wchar.h>

internal inline b32 is_newline(char c);

internal inline b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b, u64 size);
internal inline b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b);

internal inline u64 c_string_length(char *string);
internal inline u64 c_string_length(char *string, u64 max_length);

internal inline u64 c_string_length(utf8 *string);
internal inline u64 c_string_length(utf8 *string, u64 max_length);

internal inline u64 c_string_length(utf16 *string);
internal inline u64 c_string_count(utf16 *string);

internal inline u64 get_last_char_pos(String_utf8 string);

internal inline String_Const_char string_const_char(char *string);
internal inline String_Const_utf16 string_const_utf16(utf16 *string);
internal inline u64 base64_encode(u8 *out_buffer, u8 *data, u64 data_length);

internal inline String_Const_utf8 concat_str(Arena *arena, String_Const_utf8 a, String_Const_utf8 b, u64 size);

internal inline String_Const_utf8 scu8f(Arena *arena, char *format, ...);
internal inline String_Const_utf8 scu8f(Arena *arena, i32 limit, char *format, ...);

internal inline String_utf8 su8(String_Const_utf8 string);
internal inline String_utf8 su8(utf8 *string);

// Implementation
internal inline b32 is_newline(char c)
{
  b32 result = ((c == '\r') || 
                (c == '\n'));
  return(result);
}

internal inline b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b, u64 size)
{
  b32 result = true;

  expect(a.size >= size);
  expect(b.size >= size);

  for (u64 index = 0;
       index < size;
       ++index)
  {
    if (a.str[index] != b.str[index])
    {
      result = false;
      break;
    }
  }
  return(result);
}

internal inline b32 compare_string_utf8(String_Const_utf8 a, String_Const_utf8 b)
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

internal inline u64 c_string_length(char *string)
{
  u64 result = 0;
  while (string[result] != '\0')
  {
    result++;
  }
  return(result);
}

internal inline u64 c_string_length(char *string, u64 n)
{
  u64 result = 0;
  while ((n-- > 0) && string[result] != '\0')
  {
    result++;
  }
  return(result);
}

internal inline u64 c_string_length(utf8 *string)
{
  return(c_string_length((char *) string));
}

internal inline u64 c_string_length(utf8 *string, u64 max_length)
{
  return(c_string_length((char *) string, max_length));
}

internal inline u64 c_string_length(utf16 *string)
{
  u64 result = 0;
  while (string[result] != '\0') {
    result++;
  }
  return(result);
}

internal inline u64 c_string_count(utf16 *string)
{
  u64 result = wcslen(string); 
  return(result);
}

internal inline String_Const_char string_const_char(char *string)
{
  String_Const_char result = {};

  result.str = string;
  result.size = c_string_length(string);

  return(result);
}

internal inline String_Const_utf16 string_const_utf16(utf16 *string)
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
internal inline u64 base64_encode(u8 *out_buffer,
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

internal inline String_Const_utf8 concat_str(Arena *arena, String_Const_utf8 a, String_Const_utf8 b, u64 size)
{
  String_Const_utf8 res = {};

  expect(size >= (a.size + b.size));

  res.str  = push_array_zero(arena, utf8, size);
  res.size = size;

  expect(res.str != NULL);

  copy_string(res.str, a);
  copy_string(&res.str[a.size], b);

  expect(compare_string_utf8(res, a, a.size));
  expect(compare_string_utf8({&res.str[a.size], res.size - a.size}, b, b.size));

  return(res);
}
#define concat_string(arena, a, b) concat_str(arena, (a), (b), (a).size + (b).size)
#define concat_string_to_c_string(arena, a, b) concat_str(arena, (a), (b), (a).size + (b).size + 1)

internal inline String_Const_utf8 scu8f(Arena *arena, i32 limit, char *format, ...)
{
  String_Const_utf8 res = {};

  va_list args;
  va_start(args, format);

  utf8 *string_start = (utf8 *) (arena->start + arena->used);

  res.size = stbsp_vsnprintf((char *) string_start, (limit > 0) ? limit : 512, format, args);
  res.str  = string_start;

  arena_push(arena, res.size + 1);

  va_end(args);

  return(res);
}

internal inline String_Const_utf8 scu8f(Arena *arena, char *format, ...)
{
  String_Const_utf8 res = scu8f(arena, 0, format);
  return(res);
}

internal inline u64 get_last_char_pos(String_utf8 string)
{
  u64 last_char_pos = string.size;
  while ((last_char_pos >= 0) && (string.str[last_char_pos] == '\0'))
  {
    last_char_pos--;
  }
  return(last_char_pos);
}

internal inline String_utf8 su8(String_Const_utf8 string)
{
  String_utf8 res;

  res.string = string;
  res.cap    = string.size;

  return(res);
}

internal inline String_utf8 su8(utf8 *string)
{
  String_utf8 res;

  res.str = string;
  res.cap = res.size = c_string_length(string);

  return(res);
}

#define TRADER_STRING_H
#endif
