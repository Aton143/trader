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

struct Render_Context;

extern Asset_Handle render_make_texture(Render_Context *context, void *texture_data, u64 width, u64 height, u64 channels);
extern Rect_f32 render_get_client_rect();

#define TRADER_RENDER_H
#endif
