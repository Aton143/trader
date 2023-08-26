#include "trader_shapes.h"

internal void put_quad(Vertex_Buffer_Element **vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color)
{
  Vertex_Buffer_Element *cur_vertex = *vertices;
  V2_f32 solid_color_uv = render_get_solid_color_uv();

  // NOTE(antonio): top triangle
  *cur_vertex++ = 
  {
    tl,
    color,
    solid_color_uv
  };

  *cur_vertex++ = 
  {
    tr,
    color,
    solid_color_uv
  };

  *cur_vertex++ = 
  {
    bl,
    color,
    solid_color_uv
  };

  // NOTE(antonio): tom triangle
  *cur_vertex++ = 
  {
    tr,
    color,
    solid_color_uv
  };

  *cur_vertex++ = 
  {
    bl,
    color,
    solid_color_uv
  };

  *cur_vertex++ = 
  {
    br,
    color,
    solid_color_uv
  };

  *vertices = cur_vertex;
}

// NOTE(antonio): TRIANGLE FAN? 
internal Vertex_Buffer_Element *make_cylinder(Arena *render_arena,
                                              f32    base_radius,
                                              f32    top_radius,
                                              f32    height,
                                              u32    sector_count,
                                              u32    stack_count)
{
  expect(base_radius  > 0.0f);
  expect(top_radius   > 0.0f);
  expect(height       > 0.0f);
  expect(sector_count > 0);
  expect(stack_count  > 0);

  u32 vertex_count = (sector_count * stack_count * vertices_per_quad) + (2 * vertices_per_triangle * sector_count);
  Vertex_Buffer_Element *vertices = push_array_zero(render_arena, Vertex_Buffer_Element, vertex_count);
  Vertex_Buffer_Element *cur_vertex = vertices;

  f32 sector_angle_step = 1.0f / sector_count;
  f32 t_step            = 1.0f / stack_count;

  V2_f32 solid_color_uv = render_get_solid_color_uv();

  for (u32 sector_index = 0;
       sector_index < sector_count;
       ++sector_index)
  {
    f32 angle_start = sector_angle_step * sector_index;
    f32 angle_end   = sector_angle_step * (sector_index + 1);

    f32 sine_start = -sin01f(angle_start);
    f32 sine_end   = -sin01f(angle_end);

    f32 cosine_start = cos01f(angle_start);
    f32 cosine_end   = cos01f(angle_end);

    V4_f32 top_start = V4(cosine_start * top_radius,
                           height / 2.0f,
                           sine_start * top_radius, 1.0f);

    V4_f32 top_end = V4(cosine_end * top_radius,
                           height / 2.0f,
                           sine_end * top_radius, 1.0f);

    V4_f32 base_start = V4(cosine_start * base_radius,
                           -height / 2.0f,
                           sine_start * base_radius, 1.0f);

    V4_f32 base_end = V4(cosine_end * base_radius,
                           -height / 2.0f,
                           sine_end * base_radius, 1.0f);

    V4_f32 color = scale(angle_start, rgba_white);
    color.w = 1.0f;

    // NOTE(antonio): top
    *cur_vertex++ = 
    {
      V4(0.0f, top_start.y, 0, 1.0f),
      color,
      solid_color_uv
    };

    *cur_vertex++ = 
    {
      V4(top_start.x, top_start.y, top_start.z, 1.0f),
      color,
      solid_color_uv
    };

    *cur_vertex++ = 
    {
      V4(top_end.x, top_start.y, top_end.z, 1.0f),
      color,
      solid_color_uv
    };

    // NOTE(antonio): bottom
    *cur_vertex++ = 
    {
      V4(0.0f, base_start.y, 0, 1.0f),
      color,
      solid_color_uv
    };

    *cur_vertex++ = 
    {
      V4(base_start.x, base_start.y, base_start.z, 1.0f),
      color,
      solid_color_uv
    };

    *cur_vertex++ = 
    {
      V4(base_end.x, base_start.y, base_end.z, 1.0f),
      color,
      solid_color_uv
    };

    for (u32 stack_index = 0;
         stack_index < stack_count;
         ++stack_index)
    {
      f32 t0 = stack_index * t_step;
      f32 t1 = (stack_index + 1) * t_step;

      V4_f32 tl = wide_lerp(top_start, t0, base_start);
      V4_f32 tr = wide_lerp(top_end,   t0, base_end);

      V4_f32 bl = wide_lerp(top_start, t1, base_start);
      V4_f32 br = wide_lerp(top_end,   t1, base_end);

      put_quad(&cur_vertex, tl, tr, bl, br, color);
    }
  }

  return(vertices);
}
