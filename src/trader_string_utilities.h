#ifndef TRADER_STRING_H

#include <wchar.h>
#include <stdarg.h>

#define str_from_lit(s, t) concat(String_, t) {(t *) (s), (sizeof(s) - sizeof(*s)), (sizeof(s) - sizeof(*s))}
#define string_literal_init(s) {(s), sizeof(s) - sizeof(*s)}
#define string_literal_init_type(s, t) concat(String_Const_, t) {(t *) (s), sizeof(s) - 1}
#define string_from_c_string(type, s, capacity) {(type *) s, get_length_c_string((type *) s), (capacity)}
#define string_from_fixed_size(type, buffer) {(type *) (buffer), 0, sizeof(buffer) - sizeof(type)}

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

internal inline String_Const_utf8 copy_str(Arena *arena, Buffer buf);
internal inline String_Const_utf8 copy_str(Arena *arena, String_Const_utf8 str);
internal inline String_Const_utf8 concat_str(Arena *arena, String_Const_utf8 a, String_Const_utf8 b, u64 size);

internal inline String_Const_utf8 scu8(utf8 *string, u64 string_length);
internal inline String_Const_utf8 scu8f(Arena *arena, char *format, ...);
internal inline String_Const_utf8 scu8f(Arena *arena, i32 limit, char *format, ...);

internal inline String_utf8 su8(String_Const_utf8 string);
internal inline String_utf8 su8(utf8 *string);

#define TRADER_STRING_H
#endif
