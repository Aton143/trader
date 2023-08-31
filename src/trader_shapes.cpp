#include "trader_shapes.h"

internal void put_quad(Vertex_Buffer_Element **vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color)
{
  Vertex_Buffer_Element *cur_vertex = *vertices;
  V2_f32 solid_color_uv = render_get_solid_color_uv();

  // NOTE(antonio): top triangle
  *cur_vertex++ = 
    vbe(
        tl,
        color,
        solid_color_uv
       );

  *cur_vertex++ = 
    vbe(
        tr,
        color,
        solid_color_uv
       );

  *cur_vertex++ = 
    vbe(
        bl,
        color,
        solid_color_uv
       );

  // NOTE(antonio): tom triangle
  *cur_vertex++ = 
    vbe(
        tr,
        color,
        solid_color_uv
       );

  *cur_vertex++ = 
    vbe(
        bl,
        color,
        solid_color_uv
       );

  *cur_vertex++ = 
    vbe(
        br,
        color,
        solid_color_uv
       );

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
  expect(base_radius  >= 0.0f);
  expect(top_radius   >= 0.0f);
  expect(height       >= 0.0f);
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
      vbe(
          V4(0.0f, top_start.y, 0, 1.0f),
          color,
          solid_color_uv
         );

    *cur_vertex++ = 
      vbe(
          V4(top_start.x, top_start.y, top_start.z, 1.0f),
          color,
          solid_color_uv
         );

    *cur_vertex++ = 
      vbe(
          V4(top_end.x, top_start.y, top_end.z, 1.0f),
          color,
          solid_color_uv
         );

    // NOTE(antonio): bottom
    *cur_vertex++ = 
      vbe(
          V4(0.0f, base_start.y, 0, 1.0f),
          color,
          solid_color_uv
         );

    *cur_vertex++ = 
      vbe(
          V4(base_start.x, base_start.y, base_start.z, 1.0f),
          color,
          solid_color_uv
         );

    *cur_vertex++ = 
      vbe(
          V4(base_end.x, base_start.y, base_end.z, 1.0f),
          color,
          solid_color_uv
         );

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

internal Vertex_Buffer_Element *make_cylinder_along_path(Arena  *render_data,
                                                         V3_f32 *points,
                                                         u32     point_count,
                                                         f32     radius,
                                                         u32     sector_count)
{
  expect(points != NULL);
  expect(point_count > 1);
  expect(radius >= 0.0f);

  u32 vertex_count = (2 * vertices_per_triangle * sector_count) +
                     (vertices_per_quad * sector_count * (point_count - 1));
  Vertex_Buffer_Element *vertices = push_array(render_data, Vertex_Buffer_Element, vertex_count);

  Arena *temp_arena = get_temp_arena();

  V4_f32 *prev_ring = push_array_zero(temp_arena, V4_f32, sector_count);
  V4_f32 *cur_ring  = push_array_zero(temp_arena, V4_f32, sector_count);

  Vertex_Buffer_Element *cur_vertex = vertices;
  V2_f32 solid_color_uv = render_get_solid_color_uv();

  f32 sector_angle_step = 1.0f / sector_count;

  V3_f32 cross_v = V3(0.0f, 1.0f, 0.0f);

  V4_f32 c4 = V4(points[0], 1.0f);
  V3_f32 v  = normalize(subtract(points[1], points[0]));
  V3_f32 n  = cross(v, V3(0.0f, 1.0f, 0.0f));

  // NOTE(antonio): cap
  for (u32 sector_index = 0;
       sector_index < sector_count;
       ++sector_index)
  {
    f32 angle_start = sector_angle_step * sector_index;
    f32 angle_end   = sector_angle_step * (sector_index + 1);

    // TODO(antonio): too many copies!
    Matrix_f32_4x4 rotation_angle_start = matrix4x4_rotate_about_v_rodrigues(v, angle_start);
    Matrix_f32_4x4 rotation_angle_end   = matrix4x4_rotate_about_v_rodrigues(v, angle_end);

    V4_f32 v_start = transform(rotation_angle_start, V4(n, 1.0f));
    V4_f32 v_end   = transform(rotation_angle_end,   V4(n, 1.0f));

    cur_ring[sector_index]     = add(c4, scale(radius, v_start));
    cur_ring[sector_index + 1] = add(c4, scale(radius, v_end));

    RGBA_f32 color = scale((((f32) sector_index) / ((f32) sector_count)), rgba_white);
    color.a = 1.0f;

    *cur_vertex++ = 
      vbe(
      c4,
      color,
      solid_color_uv
      );

    *cur_vertex++ = 
      vbe(
      cur_ring[sector_index],
      color,
      solid_color_uv
      );

    *cur_vertex++ = 
      vbe(
      cur_ring[sector_index + 1],
      color,
      solid_color_uv
      );
  }

  swap(V4_f32 *, cur_ring, prev_ring);

  u32 i = 1;
  while (i < point_count)
  {
    V3_f32 c = points[i];

    V3_f32 to_c    = normalize(subtract(c, points[i - 1]));

    V3_f32 next    = (i < (point_count - 1)) ? points[i + 1] : c;
    V3_f32 to_next = subtract(next, c);

    V3_f32 plane_n = normalize(add(to_c, to_next));

    f32 dot_to_c_plane_n = dot(to_c, plane_n);
    expect(dot_to_c_plane_n != 0.0f);

    dot_to_c_plane_n = 1.0f / dot_to_c_plane_n;

    RGBA_f32 color = rgba_white;
    color.a = 1.0f;

    for (u32 sector_index = 0;
         sector_index < sector_count;
         ++sector_index)
    {
      f32 si_t0 = dot(subtract(c, prev_ring[sector_index]._xyz),     plane_n) * dot_to_c_plane_n;
      f32 si_t1 = dot(subtract(c, prev_ring[sector_index + 1]._xyz), plane_n) * dot_to_c_plane_n;

#if 0
      cur_ring[sector_index]     = V4(add(scale(si_t0, to_c), prev_ring[sector_index]._xyz),     1.0f);
      cur_ring[sector_index + 1] = V4(add(scale(si_t1, to_c), prev_ring[sector_index + 1]._xyz), 1.0f);

#else
      // NOTE(antonio): see below for another solution
      // v    - vector to plane
      // p    - plane vector
      // p[i] - previous vector
      // c    - next center
      // r    - radius
      //
      // p = t*v + p[i]
      // p = r * ((p - c) / ||p-c||) + c

      V3_f32 on_plane0 = add(scale(si_t0, to_c), prev_ring[sector_index]._xyz);
      on_plane0 = add(scale(radius, normalize(subtract(on_plane0, c))), c);

      V3_f32 on_plane1 = add(scale(si_t1, to_c), prev_ring[sector_index + 1]._xyz);
      on_plane1 = add(scale(radius, normalize(subtract(on_plane1, c))), c);

      cur_ring[sector_index]     = V4(on_plane0, 1.0f);
      cur_ring[sector_index + 1] = V4(on_plane1, 1.0f);
#endif

      put_quad(&cur_vertex,
               prev_ring[sector_index],
               prev_ring[sector_index + 1],
               cur_ring[sector_index],
               cur_ring[sector_index + 1],
               color);

      color = wide_lerp(color, 0.5f, rgba(1.0f, 1.0f, 0.0, 1.0f));
    }

    swap(V4_f32 *, cur_ring, prev_ring);

    i++;
  }

  return(vertices);
}

internal Vertex_Buffer_Element *make_player(Arena *render_data)
{
  const u32 player_triangle_count = 6;
  u32 vertex_count = (vertices_per_triangle * player_triangle_count);

  Vertex_Buffer_Element *vertices   = push_array(render_data, Vertex_Buffer_Element, vertex_count);
  Vertex_Buffer_Element *cur_vertex = vertices; 

  V2_f32 solid_color_uv = render_get_solid_color_uv();
  RGBA_f32 color = rgba_white;

  V4_f32 back_top = V4( 0.0f, 0.05f,  0.50f, 1.0f);
  V4_f32 front    = V4( 0.0f, 0.0f,  -0.50f, 1.0f);
  V4_f32 w0       = V4(-0.5f, 0.0f,   0.50f, 1.0f);

  // LT Hull
  V4_f32 normal = V4(triangle_normal_ccw(back_top._xyz, front._xyz, w0._xyz), 1.0f);
  *cur_vertex++ = vbe(back_top, color, solid_color_uv, normal);
  *cur_vertex++ = vbe(front,    color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,       color, solid_color_uv, normal);

  color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(reflect_about_xz(back_top)._xyz, front._xyz, w0._xyz), 1.0f);

  // LB Hull
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(front,                      color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,                         color, solid_color_uv, normal);

  color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(reflect_about_xz(back_top)._xyz, back_top._xyz, w0._xyz), 1.0f);

  // LBack Hull
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(back_top,                   color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,                         color, solid_color_uv, normal);

  color = rgba_white;
  normal = V4(triangle_normal_ccw(front._xyz, back_top._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RT Hull
  *cur_vertex++ = vbe(front,                color, solid_color_uv, normal);
  *cur_vertex++ = vbe(back_top,             color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0), color, solid_color_uv, normal);

  color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(front._xyz, reflect_about_xz(back_top)._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RB Hull
  *cur_vertex++ = vbe(front,                      color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0),       color, solid_color_uv, normal);

  color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(back_top._xyz, reflect_about_xz(back_top)._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RBack Hull
  *cur_vertex++ = vbe(back_top,                   color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0),       color, solid_color_uv, normal);

  return(vertices);
}
