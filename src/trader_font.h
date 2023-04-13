#ifndef TRADER_FONT_H

#define stbtt_uint8  u8
#define stbtt_int8   i8
#define stbtt_uint16 u16
#define stbtt_int16  i16
#define stbtt_uint32 u32
#define stbtt_int32  i32

struct Font_Bitmap {
  u8 *alpha;

  u32 width;
  u32 height;
};

struct Font_Data {
  Font_Bitmap       bitmap;
  stbtt_packedchar *char_data;

  f32               heights[4];
  u32               char_data_set_counts[4];
  u32               height_count;
};

global File_Buffer default_font = {};

global_const utf32 starting_code_point = 32;  // ' '
global_const utf32 ending_code_point   = 126; // '~'

b32 font_initialize(Arena *arena,
                           Font_Data *font_data,
                           File_Buffer *font_buffer,
                           f32 *font_heights,
                           u32 font_height_count,
                           u32 bitmap_width = 512,
                           u32 bitmap_height = 512);

b32 font_get_char_quad_and_advance(Font_Data *font_data,
                                   Quad *out_quad,
                                   f32 *in_out_x,
                                   f32 *in_out_y,
                                   utf32 code_point,
                                   u32 font_choice = 0);

// implementation
b32 font_initialize(Arena *arena,
                    Font_Data *font_data,
                    File_Buffer *font_buffer,
                    f32 *font_heights,
                    u32 font_height_count,
                    u32 bitmap_width,
                    u32 bitmap_height)
{
  b32 result = false;
  stbtt_fontinfo font_info = {};

  {
    Font_Data _fd;
    assert((font_heights != NULL) && (0 < font_height_count) && font_height_count < array_count(_fd.heights));
  }

  i32 font_count = stbtt_GetNumberOfFonts(font_buffer->data);
  if (font_count > 0) 
  {
    i32 font_offset = stbtt_GetFontOffsetForIndex(font_buffer->data, 0);

    // NOTE(antonio): fallback font
    if (!stbtt_InitFont(&font_info, font_buffer->data, font_offset))
    {
      assert((default_font.data != NULL) || !"expected a default font at least...");

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

    u32 code_point_count      = (ending_code_point - starting_code_point) + 1;

    stbtt_packedchar *packed_chars =
      (stbtt_packedchar *) push_array(arena, stbtt_packedchar, font_height_count * code_point_count);
    if (!packed_chars)
    {
      assert(!"expected arena allocation to succeed");
    }

    stbtt_packedchar *packed_chars_start = packed_chars;
    for (u32 font_height_index = 0;
         font_height_index < font_height_count;
         ++font_height_index)
    {
      f32 font_height = font_heights[font_height_index];

      if (font_height < 15.0f) stbtt_PackSetOversampling(&pack_context, 2, 2);

      if (!stbtt_PackFontRange(&pack_context,
                               font_buffer->data, font_offset, font_height,
                               starting_code_point, ending_code_point, packed_chars_start))
      {
        // TODO(antonio): resize things accordingly
        assert(!"expected bitmap size to work");
      }

      packed_chars_start += code_point_count;
    }

    stbtt_PackEnd(&pack_context);

    if (font_data != NULL)
    {
      zero_struct(font_data);

      font_data->bitmap.alpha        = bitmap_data;
      font_data->bitmap.width        = bitmap_width;
      font_data->bitmap.height       = bitmap_height;

      font_data->char_data           = packed_chars;
      font_data->height_count        = font_height_count;

      copy_memory_block(font_data->heights, font_heights, font_height_count);

      for (u32 set_index = 0;
           set_index < font_height_count;
           ++set_index)
      {
        font_data->char_data_set_counts[set_index] = code_point_count;
      }

      result = true;
    }
  }

  return(result);
}

b32 font_get_char_quad_and_advance(Font_Data *font_data, Quad *out_quad, f32 *x, f32 *y, utf32 code_point, u32 font_choice)
{
  b32 result = false;
  assert(out_quad != NULL);
  assert(font_data != NULL);

  if ((starting_code_point <= code_point) && (code_point <= ending_code_point))
  {
    stbtt_aligned_quad temp_quad = {};

    utf32 converted_code_point = code_point - starting_code_point;
    stbtt_packedchar *char_data = font_data->char_data;

    for (u32 set_index = 0;
         set_index < font_choice;
         ++set_index)
    {
      char_data += font_data->char_data_set_counts[set_index];
    }

    // TODO(antonio): integer align?
    stbtt_GetPackedQuad(char_data,
                        font_data->bitmap.width,
                        font_data->bitmap.height,
                        converted_code_point,
                        x, y, 
                        &temp_quad, 0);

    {
      *out_quad = {};

      out_quad->pos.x0 = temp_quad.x0;
      out_quad->pos.y0 = temp_quad.y0;
      out_quad->pos.x1 = temp_quad.x1;
      out_quad->pos.y1 = temp_quad.y1;

      out_quad->uv.x0  = temp_quad.s0;
      out_quad->uv.y0  = temp_quad.t0;
      out_quad->uv.x1  = temp_quad.s1;
      out_quad->uv.y1  = temp_quad.t1;
    }

    result = true;
  }

  return(result);
}

#define TRADER_FONT_H
#endif
