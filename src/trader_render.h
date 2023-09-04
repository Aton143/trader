#ifndef TRADER_RENDER_H

typedef u32 Render_Command_Kind;
enum
{
  rck_none,
  rck_draw,
  rck_clear,
  rck_count,
};

struct RCK_Draw
{
  u32  buffer_id;
  u32  topology;

  u32  per_vertex_size;
  u32  vertex_count;

  u8  *vertex_data;

  Vertex_Shader vertex_shader;
  Pixel_Shader  pixel_shader;

  Texture       textures[2];

  u8   constant_buffer_data[256];
};

struct RCK_Clear
{
  void *frame_buffer;
  void *depth_stencil_buffer;
};

// TODO(antonio): move to discriminated union based system?
struct Render_Command
{
  Render_Command_Kind kind;

  union
  {
    RCK_Draw  draw;
    RCK_Clear clear;
  };
};

#pragma pack(push, 4)

struct Constant_Buffer
{
  f32 atlas_width, atlas_height;
  f32 client_width, client_height;

  union {
    // NOTE(antonio): for instance-based rendering
    struct {
      f32 client_width;
      f32 client_height;
    };

    // NOTE(antonio): for vertex-based rendering
    struct {
      Matrix_f32_4x4 model;
      Matrix_f32_4x4 view;
      Matrix_f32_4x4 projection;
    };
  };
};

struct Instance_Buffer_Element
{
  union
  {
    Rect_f32  size;
    struct
    {
      V2_f32  size_top_left;
      V2_f32  size_bottom_right;
    };
  };
  union
  {
    RGBA_f32 color[4];
    struct
    {
      RGBA_f32 color_top_left;
      RGBA_f32 color_top_right;
      RGBA_f32 color_bottom_left;
      RGBA_f32 color_bottom_right;
    };
  };
  V3_f32    pos;
  f32       corner_radius;
  f32       edge_softness;
  f32       border_thickness;
  union
  {
    Rect_f32  uv;
    struct
    {
      V2_f32  uv_top_left;
      V2_f32  uv_bottom_right;
    };
  };
};
#pragma pack(pop)

struct Vertex_Buffer_Element
{
  V4_f32   position;
  RGBA_f32 color;
  V4_f32   normal;
  V2_f32   uv;
};

struct Alpha_Bitmap {
  u8 *alpha;

  f32 width;
  f32 height;
};
typedef Alpha_Bitmap Font_Bitmap;

struct Bitmap
{
  u8 *data;

  f32 width;
  f32 height;
  u32 channels;
};

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

typedef u32 Shader_Kind;
enum
{
  Shader_Kind_None,
  Shader_Kind_Vertex,
  Shader_Kind_Pixel,
  Shader_Kind_Count,
};

struct Common_Render_Context
{
  Texture_Atlas *atlas;
  Arena          render_data;
  Arena          triangle_render_data;
  V2_f32         vertex_render_dimensions;
  Rect_f32       client_rect;
  File_Buffer    default_font;

  V2_f32         dpi;
};

#include "trader_platform.h"

global_const u32 render_data_size          = mb(1);
global_const u32 triangle_render_data_size = mb(1);
global_const u32 vertices_per_triangle     = 3;
global_const u32 vertices_per_quad         = 6;

global_const utf32 starting_code_point = 32;  // ' '
global_const utf32 ending_code_point   = 126; // '~'

// extern Asset_Handle render_make_texture(void *texture_data, u64 width, u64 height, u64 channels);
internal inline Render_Context        *render_get_context(void);
internal inline Common_Render_Context *render_get_common_context(void);
internal inline void                   render_set_client_rect(Rect_f32 new_rect);
internal inline Rect_f32               render_get_client_rect(void);
internal inline Rect_f32               render_get_solid_color_rect(void);
internal inline V2_f32                 render_get_solid_color_uv(void);

internal void *render_load_vertex_shader(Handle *shader_handle, Vertex_Shader *shader, b32 force = false);
internal void  render_load_pixel_shader(Handle *shader_handle, Pixel_Shader *shader, b32 force = false);
internal void  render_debug_print_compile_errors(void *data);

internal void  render_push_line_instance(V2_f32 line_start, f32 length, f32 dir_x, f32 dir_y,
                                         RGBA_f32 color = rgba_white); 

internal void  render_create_cubemap(Bitmap *bitmaps, u32 bitmap_count, void *out_textures);

internal inline Vertex_Buffer_Element vbe(V4_f32 position, RGBA_f32 color, V2_f32 uv, V4_f32 normal = v4_zero);
internal Vertex_Buffer_Element *render_push_triangles(u64 triangle_count);
internal void render_data_to_lines(V2_f32 *points, u64 point_count);

internal i64 render_get_font_height_index(f32 font_height);
internal i64 render_get_packed_char_start(f32 font_height);

internal void render_get_text_dimensions(f32 *x, f32 *y, Rect_f32 bounds, String_Const_utf8 string, i64 up_to);
internal void render_draw_text(Arena *render_arena, f32 *x, f32 *y, RGBA_f32 color, Rect_f32 bounds, utf8 *format, ...);

internal b32 render_atlas_initialize(Arena         *arena,
                                     Texture_Atlas *atlas,
                                     File_Buffer   *font_data,
                                     f32           *font_heights,
                                     u32            font_height_count,
                                     u32            bitmap_width,
                                     u32            bitmap_height);

internal THREAD_RETURN THREAD_CALL_CONVENTION render_thread_proc(void *args);

#define TRADER_RENDER_H
#endif
