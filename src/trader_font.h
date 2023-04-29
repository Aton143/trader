#ifndef TRADER_FONT_H

#define stbtt_uint8  u8
#define stbtt_int8   i8
#define stbtt_uint16 u16
#define stbtt_int16  i16
#define stbtt_uint32 u32
#define stbtt_int32  i32

struct Font_Data {
  Font_Bitmap       bitmap;
  stbtt_packedchar *char_data;

  f32               heights[4];
  u32               char_data_set_counts[4];
  u32               height_count;
};

/*
global File_Buffer default_font = {};

global_const utf32 starting_code_point = 32;  // ' '
global_const utf32 ending_code_point   = 126; // '~'
*/

unimplemented b32 font_get_char_quad_and_advance(Font_Data *font_data,
                                                 Quad      *out_quad,
                                                 f32       *in_out_x,
                                                 f32       *in_out_y,
                                                 utf32      code_point,
                                                 u32        font_choice = 0);

// implementation
/*
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
*/

#define TRADER_FONT_H
#endif
