#ifndef TRADER_RENDER_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "./foreign/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "./foreign/stb_truetype.h"

union Color_f32 {
  struct {
    f32 r, g, b, a;
  };
  f32 vals[4];
};

struct Constant_Buffer
{
  f32 client_width, client_height;
  f32 atlas_width,  atlas_height;
};

struct Instance_Buffer_Element
{
  Rect_f32  size;
  Color_f32 color;
  V3_f32    pos;
  Rect_f32  uv;
};

struct Alpha_Bitmap {
  u8 *alpha;

  f32 width;
  f32 height;
};
typedef Alpha_Bitmap Font_Bitmap;

// NOTE(antonio): includes fonts, solid color, and others if necessary
struct Texture_Atlas
{
  Font_Bitmap       bitmap;

  stbtt_fontinfo    font_info;
  stbtt_packedchar *char_data;

  f32               heights[4];
  u32               char_data_set_counts[4];
  u32               height_count;

  Rect_i16          solid_color_rect;
};

struct Vertex_Shader;
struct Pixel_Shader;

global_const u64 render_data_size = mb(1);
global File_Buffer default_font = {};

global_const utf32 starting_code_point = 32;  // ' '
global_const utf32 ending_code_point   = 126; // '~'

// extern Asset_Handle render_make_texture(void *texture_data, u64 width, u64 height, u64 channels);
internal Rect_f32 render_get_client_rect();

internal void *render_load_vertex_shader(Handle *shader_handle, Vertex_Shader *shader, b32 force = false);
internal void  render_load_pixel_shader(Handle *shader_handle, Pixel_Shader *shader, b32 force = false);

internal void render_draw_text(utf8 *text, u64 text_size, f32 *x, f32 *y);

internal b32 render_atlas_initialize(Arena         *arena,
                                     Texture_Atlas *atlas,
                                     File_Buffer   *font_data,
                                     f32           *font_heights,
                                     u32            font_height_count,
                                     u32            bitmap_width,
                                     u32            bitmap_height);

// implementation
internal b32 render_atlas_initialize(Arena         *arena,
                                     Texture_Atlas *atlas,
                                     File_Buffer   *font_data,
                                     f32           *font_heights,
                                     u32            font_height_count,
                                     u32            bitmap_width,
                                     u32            bitmap_height)
{
  b32 result = true;

  assert((font_heights != NULL) &&
         ((0 < font_height_count) &&
          (font_height_count <= member_array_count(Texture_Atlas, heights))));

  i32 font_count = stbtt_GetNumberOfFonts(font_data->data);
  if (font_count > 0) 
  {
    stbtt_fontinfo font_info = {};
    i32 font_offset = stbtt_GetFontOffsetForIndex(font_data->data, 0);

    // NOTE(antonio): fallback font
    if (!stbtt_InitFont(&font_info, font_data->data, font_offset))
    {
      assert((default_font.data != NULL) && "expected a default font at least...");

      font_data = &default_font;
      stbtt_InitFont(&font_info, font_data->data, 0);
    }

    // NOTE(antonio): the bitmap only encodes alpha
    u8 *bitmap_data = (u8 *) arena_push(arena, bitmap_width * bitmap_height);

    if (bitmap_data == NULL)
    {
      assert(!"expected arena allocation to succeed");
    }

    stbtt_pack_context pack_context;
    stbtt_PackBegin(&pack_context, bitmap_data, bitmap_width, bitmap_height, 0, 1, NULL);
    {
      u32 code_point_count = (ending_code_point - starting_code_point) + 1;

      // NOTE(antonio): the extra is the solid color glyph
      u32               rect_count        = (code_point_count * font_height_count) + 1;
      u32               packed_char_count = rect_count - 1;

      stbtt_packedchar *packed_chars      = push_array_zero(arena, stbtt_packedchar, packed_char_count);
      stbrp_rect       *rects             = push_array(arena, stbrp_rect, rect_count);

      stbtt_pack_range pack_ranges[4] = {};

      stbrp_rect       *rects_being_packed        = rects;
      stbtt_packedchar *packed_chars_being_packed = packed_chars;

      for (u32 pack_range_index = 0;
           pack_range_index < font_height_count;
           ++pack_range_index)
      {
        pack_ranges[pack_range_index].font_size                        = font_heights[pack_range_index];
        pack_ranges[pack_range_index].first_unicode_codepoint_in_range = starting_code_point;
        pack_ranges[pack_range_index].array_of_unicode_codepoints      = NULL;
        pack_ranges[pack_range_index].num_chars                        = code_point_count;
        pack_ranges[pack_range_index].chardata_for_range               = packed_chars_being_packed;
        
        if (font_heights[pack_range_index] < 15.0f) stbtt_PackSetOversampling(&pack_context, 2, 2);
        else                                        stbtt_PackSetOversampling(&pack_context, 1, 1);

        stbtt_PackFontRangesGatherRects(&pack_context,
                                        &font_info,
                                        &pack_ranges[pack_range_index],
                                        1,
                                        rects_being_packed);

        rects_being_packed        += code_point_count;
        packed_chars_being_packed += code_point_count;
      }

      stbrp_rect *residue_rect = rects_being_packed;
      {
        residue_rect->w = 1;
        residue_rect->h = 1;
      }

      stbtt_PackFontRangesPackRects(&pack_context, rects, rect_count);
      i32 stb_result = stbtt_PackFontRangesRenderIntoRects(&pack_context,
                                                           &font_info,
                                                           pack_ranges,
                                                           font_height_count,
                                                           rects);
      result = (b32) stb_result;

      // render the rest
      bitmap_data[(residue_rect->y * bitmap_width) + residue_rect->x] = 0xff;

      pop_array(arena, stbrp_rect, rect_count);
      if (result)
      {
        atlas->bitmap.alpha  = bitmap_data;
        atlas->bitmap.width  = (f32) bitmap_width;
        atlas->bitmap.height = (f32) bitmap_height;

        copy_struct(&atlas->font_info, &font_info);

        for (u64 packed_char_index = 0;
             packed_char_index < packed_char_count;
             ++packed_char_index)
        {
          // stbtt_packedchar *packed_char = packed_chars + packed_char_index;
          // packed_char->yoff *= -1.0f;
        }
        atlas->char_data = packed_chars;


        copy_array(atlas->heights, font_heights, font_height_count);

        for (u32 count_index = 0;
             count_index < font_height_count;
             ++count_index)
        {
          atlas->char_data_set_counts[count_index] = code_point_count;
        }

        atlas->height_count    = font_height_count;

        atlas->solid_color_rect.x0 = (i16) residue_rect->x; 
        atlas->solid_color_rect.y0 = (i16) residue_rect->y;
        atlas->solid_color_rect.x1 = (i16) (atlas->solid_color_rect.x0 + residue_rect->w);
        atlas->solid_color_rect.y1 = (i16) (atlas->solid_color_rect.y0 + residue_rect->h);
      }
      else
      {
        pop_array(arena, stbtt_packedchar, packed_char_count);
        arena_pop(arena, bitmap_width * bitmap_height);
      }
    }
    stbtt_PackEnd(&pack_context);
  }

  return(result);
}

#define TRADER_RENDER_H
#endif
