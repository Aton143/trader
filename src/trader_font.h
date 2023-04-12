#ifndef TRADER_FONT_H

#define stbtt_uint8  u8
#define stbtt_int8   i8
#define stbtt_uint16 u16
#define stbtt_int16  i16
#define stbtt_uint32 u32
#define stbtt_int32  i32

global File_Buffer default_font = {};
b32 trader_font_initialize(Arena *arena, File_Buffer *font_buffer,
                           f32 font_height, u32 bitmap_width = 512, u32 bitmap_height = 512);

// implementation
b32 trader_font_initialize(Arena *arena, File_Buffer *font_buffer,
                           f32 font_height, u32 bitmap_width, u32 bitmap_height)
{
  b32 result = false;
  stbtt_fontinfo font_info = {};
  unused(arena);

  i32 font_count = stbtt_GetNumberOfFonts(font_buffer->data);
  if (font_count > 0) 
  {
    i32 font_offset = stbtt_GetFontOffsetForIndex(font_buffer->data, 0);

    // NOTE(antonio): fallback font
    if (!stbtt_InitFont(&font_info, font_buffer->data, font_offset))
    {
      font_buffer = &default_font;
      stbtt_InitFont(&font_info, font_buffer->data, 0);
    }

    // NOTE(antonio): the bitmap only encodes alpha
    u8 *bitmap_data = (u8 *) arena_push(arena, bitmap_width * bitmap_height);
    if (bitmap_data == NULL)
    {
      assert(!"expected arena allocation to succeed");
    }

    stbtt_pack_context pack_context;
    stbtt_PackBegin(&pack_context, bitmap_data, bitmap_width, bitmap_height, 0, 1, NULL);

    utf32 starting_code_point = 32;  // NOTE(antonio): represents space
    utf32 ending_code_point   = 126; // NOTE(antonio): represents tilde

    stbtt_packedchar *packed_chars = (stbtt_packedchar *) push_array(arena, stbtt_packedchar,
                                                                     ending_code_point - starting_code_point + 1);
    if (!packed_chars)
    {
      assert(!"expected arena allocation to succeed");
    }


    // TODO(antonio): oversampling?
    if (stbtt_PackFontRange(&pack_context, font_buffer->data, font_offset, font_height,
                            starting_code_point, ending_code_point, packed_chars))
    {

    }


    stbtt_PackEnd(&pack_context);
  }

  return(result);
}

#define TRADER_FONT_H
#endif
