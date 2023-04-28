#ifndef TRADER_RENDER_H

union Color_f32 {
  struct {
    f32 r, g, b, a;
  };
  f32 vals[4];
};

struct Constant_Buffer
{
  f32 client_width, client_height;
  f32 font_atlas_width, font_atlas_height;
};

struct Instance_Buffer_Element
{
  Rect_f32  size;
  Color_f32 color;
  V2_f32    pos;
  Rect_f32  uv;
};

struct Alpha_Bitmap {
  u8 *alpha;

  u32 width;
  u32 height;
};
typedef Alpha_Bitmap Font_Bitmap;

struct Render_Context;

extern Asset_Handle render_make_texture(Render_Context *context, void *texture_data, u64 width, u64 height, u64 channels);
extern Rect_f32 render_get_client_rect();

// NOTE(antonio): includes fonts, solid color, and others if necessary
struct Texture_Atlas
{
  Font_Bitmap       bitmap;
  stbtt_packedchar *char_data;

  f32               heights[4];
  u32               char_data_set_counts[4];
  u32               height_count;

  V2_i16            solid_color;
};

internal b32 render_atlas_initialize(Arena         *arena,
                                     Texture_Atlas *atlas,
                                     File_Buffer   *font_data,
                                     u32            bitmap_width,
                                     u32            bitmap_height)
{
  b32 result = true;

  unused(arena);
  unused(atlas);
  unused(font_data);
  unused(bitmap_width);
  unused(bitmap_height);

  return(result);
}

#define TRADER_RENDER_H
#endif
