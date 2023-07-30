#if !defined(TRADER_UNICODE_H)

global_const String_utf8 word_separators = str_from_lit(" \n\t`~!@#$%^&*()-=+[{]}\\|;:'\",.<>/?", utf8);

// TODO(antonio): may be worth using the same length idea for all of these
// TODO(antonio): error-checking
internal inline b32 unicode_utf8_is_start(utf8 encoding_char);

// NOTE(antonio): can stay in the same pos
internal inline i64 unicode_utf8_get_next_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_start_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

// NOTE(antonio): will "advance" (if in bounds)
internal inline i64 unicode_utf8_advance_char_pos(utf8 *start, i64 start_pos, i64 encoding_size_in_bytes, i32 dir);

internal inline i64 unicode_utf8_get_next_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_char_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_advance_by_delim_spans(utf8        *encoding_start,
                                                        i64          encoding_pos,
                                                        i64          encoding_size_in_bytes,
                                                        i32          dir,
                                                        i32          delims_to_cross_count = 1,
                                                        String_utf8  delims = word_separators);

internal inline i64 unicode_utf8_get_next_word_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);
internal inline i64 unicode_utf8_get_prev_word_pos(utf8 *encoding_start, i64 encoding_pos, i64 encoding_size_in_bytes);

internal inline i64 unicode_utf8_encoding_length(utf8 *encoding);
internal inline i64 unicode_utf8_encoding_length(utf8 *encoding, i64 char_count);

unimplemented inline i64 unicode_utf8_verify(utf8 *encoding, i64 encoding_length);
internal      inline i64 unicode_utf8_encode(u32  *code_points,
                                             i64   code_point_length,
                                             utf8 *put,
                                             i64   put_pos,
                                             i64   put_length,
                                             i64  *out_length_in_bytes = NULL);

unimplemented inline i64 unicode_utf16_verify(utf16 *encoding, i64 encoding_length);
unimplemented inline i64 unicode_utf16_encode(u32 *code_points,
                                              i64 code_point_length, utf16 *put, i64 put_pos, i64 put_length);

internal inline i64 unicode_utf8_get_char_pos_in_string(utf8 *encoding, i64 char_count, String_utf8 string);
internal inline b32 unicode_utf8_is_char_in_string(utf8 *encoding, i64 char_count, String_utf8 string);

internal inline u32 unicode_utf16_get_code_point(utf16 *encoding_start, i64 *encoding_pos, i64 encoding_size);
internal inline utf8 *unicode_utf8_from_utf16(Arena *arena,
                                              utf16 *from,
                                              i64    from_length_in_bytes,
                                              i64   *out_length);

global_const u16 utf16_high_surrogate_factor = 0xd800;
global_const u16 utf16_low_surrogate_factor  = 0xdc00;
global_const u16 utf16_surrogate_supplement  = 0xdc00;
#define TRADER_UNICODE_H
#endif
