#include "trader_shapes.h"

void put_quad_uvs(Vertex_Buffer_Element **vertices, V2_f32 tl, V2_f32 tr, V2_f32 bl, V2_f32 br)
{
  Vertex_Buffer_Element *cur_vertex = *vertices;

  cur_vertex->uv = tl;
  cur_vertex++;

  cur_vertex->uv = bl;
  cur_vertex++;

  cur_vertex->uv = tr;
  cur_vertex++;

  cur_vertex->uv = tr;
  cur_vertex++;

  cur_vertex->uv = bl;
  cur_vertex++;

  cur_vertex->uv = br;
  cur_vertex++;

  *vertices = cur_vertex;
}

void put_quad(Vertex_Buffer_Element **vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color, V4_f32 normal)
{
  Vertex_Buffer_Element *cur_vertex = *vertices;
  V2_f32 solid_color_uv = render_get_solid_color_uv();

  // NOTE(antonio): top triangle
  *cur_vertex++ = 
    vbe(
        tl,
        color,
        solid_color_uv,
        normal
       );

  *cur_vertex++ = 
    vbe(
        bl,
        color,
        solid_color_uv,
        normal
       );

  *cur_vertex++ = 
    vbe(
        tr,
        color,
        solid_color_uv,
        normal
       );

  // NOTE(antonio): bottom triangle
  *cur_vertex++ = 
    vbe(
        tr,
        color,
        solid_color_uv,
        normal
       );

  *cur_vertex++ = 
    vbe(
        bl,
        color,
        solid_color_uv,
        normal
       );

  *cur_vertex++ = 
    vbe(
        br,
        color,
        solid_color_uv,
        normal
       );

  *vertices = cur_vertex;
}

// NOTE(antonio): TRIANGLE FAN? 
Render_Position make_cylinder(Arena *render_data,
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
  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), vertex_count};
  Vertex_Buffer_Element *vertices = push_array_zero(render_data, Vertex_Buffer_Element, vertex_count);

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

    V4_f32 color = /*scale(angle_start,*/ rgba_white;//);
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

  return(rp);
}

Vertex_Buffer_Element *make_cylinder_along_path(Arena  *render_data,
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

Render_Position make_player(Arena *render_data)
{
  const u32 player_triangle_count = 6;
  u32 vertex_count = (vertices_per_triangle * player_triangle_count);

  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), vertex_count};
  Vertex_Buffer_Element *cur_vertex = push_array(render_data, Vertex_Buffer_Element, rp.count);

  V2_f32 solid_color_uv = render_get_solid_color_uv();
  RGBA_f32 color = rgba(0.0f, 0.0f, 0.0f, 0.0f);

  V4_f32 back_top = V4( 0.0f, 0.05f,  0.50f, 1.0f);
  V4_f32 front    = V4( 0.0f, 0.0f,  -0.50f, 1.0f);
  V4_f32 w0       = V4(-0.5f, 0.0f,   0.50f, 1.0f);

  // LT Hull
  V4_f32 normal = V4(triangle_normal_ccw(back_top._xyz, front._xyz, w0._xyz), 1.0f);
  *cur_vertex++ = vbe(back_top, color, solid_color_uv, normal);
  *cur_vertex++ = vbe(front,    color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,       color, solid_color_uv, normal);

  // color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(reflect_about_xz(back_top)._xyz, front._xyz, w0._xyz), 1.0f);

  // LB Hull
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(front,                      color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,                         color, solid_color_uv, normal);

  // color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(reflect_about_xz(back_top)._xyz, back_top._xyz, w0._xyz), 1.0f);

  // LBack Hull
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(back_top,                   color, solid_color_uv, normal);
  *cur_vertex++ = vbe(w0,                         color, solid_color_uv, normal);

  // color = rgba_white;
  normal = V4(triangle_normal_ccw(front._xyz, back_top._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RT Hull
  *cur_vertex++ = vbe(front,                color, solid_color_uv, normal);
  *cur_vertex++ = vbe(back_top,             color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0), color, solid_color_uv, normal);

  // color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(front._xyz, reflect_about_xz(back_top)._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RB Hull
  *cur_vertex++ = vbe(front,                      color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0),       color, solid_color_uv, normal);

  // color = wide_lerp(color, 0.25f, rgba_red);
  normal = V4(triangle_normal_ccw(back_top._xyz, reflect_about_xz(back_top)._xyz, reflect_about_yz(w0)._xyz), 1.0f);

  // RBack Hull
  *cur_vertex++ = vbe(back_top,                   color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_xz(back_top), color, solid_color_uv, normal);
  *cur_vertex++ = vbe(reflect_about_yz(w0),       color, solid_color_uv, normal);

  return(rp);
}

Render_Position make_cube(Arena *render_data, RGBA_f32 *face_colors)
{
  u32 face_count = 6;
  u32 cube_triangle_count = 2 * face_count;
  u32 vertex_count = (vertices_per_triangle * cube_triangle_count);

  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), vertex_count};
  Vertex_Buffer_Element *cur_vertex = push_array(render_data, Vertex_Buffer_Element, rp.count);

  V4_f32 *cur_extra = (V4_f32 *) (((u8 *) cur_vertex) + (sizeof(Vertex_Buffer_Element) / 2));
  unused(cur_extra);

  V2_f32 solid_color_uv = render_get_solid_color_uv();

  V4_f32 tl, tr, bl, br, normal;
  f32 mag = rsubcube_width / 2.0f;

  V2_f32 tl_uv = V2(0.0f, 0.0f);
  V2_f32 tr_uv = V2(1.0f, 0.0f);
  V2_f32 bl_uv = V2(0.0f, 1.0f);
  V2_f32 br_uv = V2(1.0f, 1.0f);

  // front
  tl = V4(-mag,  mag,  mag, 1.0f);
  tr = V4( mag,  mag,  mag, 1.0f);
  bl = V4(-mag, -mag,  mag, 1.0f);
  br = V4( mag, -mag,  mag, 1.0f);
  normal = V4(0.0f, 0.0f, 1.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  // back
  tl.z = tr.z = bl.z = br.z *= -1.0f;
  normal.z *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  // top
  tl = V4(-mag,  mag, -mag, 1.0f);
  tr = V4( mag,  mag, -mag, 1.0f);
  bl = V4(-mag,  mag,  mag, 1.0f);
  br = V4( mag,  mag,  mag, 1.0f);
  normal = V4(0.0f, 1.0f, 0.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  // bottom
  tl.y = tr.y = bl.y = br.y *= -1.0f;
  normal.y *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  // left
  tl = V4(-mag,  mag, -mag, 1.0f);
  tr = V4(-mag,  mag,  mag, 1.0f);
  bl = V4(-mag, -mag, -mag, 1.0f);
  br = V4(-mag, -mag,  mag, 1.0f);
  normal = V4(-1.0f, 0.0f, 0.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  // right
  tl.x = tr.x = bl.x = br.x *= -1.0f;
  normal.x *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);
  cur_vertex -= vertices_per_quad;
  put_quad_uvs(&cur_vertex, tl_uv, tr_uv, bl_uv, br_uv);

  return(rp);
}

internal inline void rcube_rotate_face(u8 *cube_faces, i32 face, i32 dir)
{
  u8 *r = cube_faces + (face * 9);
  u8 copy[9] = {};
  memcpy(copy, r, 9);

  if (dir == ccw)
  {
    r[0] = copy[2];
    r[1] = copy[5];
    r[2] = copy[8];

    r[3] = copy[1];
    r[4] = copy[4];
    r[5] = copy[7];

    r[6] = copy[0];
    r[7] = copy[3];
    r[8] = copy[6];
  }
  else
  {
    r[0] = copy[6];
    r[1] = copy[3];
    r[2] = copy[0];
        
    r[3] = copy[7];
    r[4] = copy[4];
    r[5] = copy[1];
        
    r[6] = copy[8];
    r[7] = copy[5];
    r[8] = copy[2];
  }
}

internal inline void rotate_slice(u8 *cube_faces, u8 *slice, i32 dir, Orientation orientation)
{
  u8 blocks[6] = {0, 3, 0, 6, 0, 9};

  u8 start = (dir > 0) ? 0 : 4;
  u8 adder = (dir > 0) ? 2 : -2;

  for (u8 i = 0; i < 3; ++i)
  {
    for (u8 j = 0; j < 3; ++j)
    {
#if 1
      swap(u8, cube_faces[slice[blocks[start] + j]], cube_faces[slice[blocks[start + 1] + j]]);
#else
      unused(cube_faces);
      swap(u8, slice[blocks[start] + j], slice[blocks[start + 1] + j]);
#endif
    }
    start += adder;
  }

  if (orientation == olr)
  {
    if (dir == cw)
    {
      swap(u8, cube_faces[slice[6]], cube_faces[slice[8]]);
      swap(u8, cube_faces[slice[9]], cube_faces[slice[11]]);
    }
    else
    {
      swap(u8, cube_faces[slice[3]], cube_faces[slice[5]]);
      swap(u8, cube_faces[slice[6]], cube_faces[slice[8]]);
    }
  }
}

local_const u8 face_map[6][6] = 
{
  {0, 1, 2, 3, 4, 5},
  {1, 2, 3, 0, 4, 5},
  {2, 3, 0, 1, 4, 5},
  {3, 0, 1, 2, 4, 5},
  {4, 1, 5, 3, 2, 0},
  {5, 1, 4, 3, 0, 2},
};

local_const u8 face_ccw_rot_map[6][6] = 
{
  {0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 1, 3,},
  {0, 0, 0, 0, 2, 2,},
  {0, 0, 0, 0, 3, 1,},
  {0, 3, 2, 1, 0, 2,},
  {0, 1, 2, 3, 2, 0,},
};

internal inline void rcube_remap(u8 *dest, u8 *rotate, u8 base_face)
{
  for (u32 face_index = 0;
       face_index < 6;
       ++face_index)
  {
    u32 rot_count = face_ccw_rot_map[base_face][face_index];
    while (rot_count-- > 0)
    {
      rcube_rotate_face(rotate, face_index, ccw);
    }
  }

  for (u32 face_index = 0;
       face_index < 6;
       ++face_index)
  {
    copy_memory_block(dest + (rcube_stickers_per_face * face_index),
                      rotate + (rcube_stickers_per_face * face_map[base_face][face_index]),
                      rcube_stickers_per_face);
  }
}

internal void rcube_do_move(RCube *cube, RCube_Move_Direction *move)
{
  local_persist u8 rcube_faces_rot_copy[54];
  local_persist u8 rcube_mapped_copy[54];
  copy_memory_block(rcube_faces_rot_copy, cube->faces, sizeof(cube->faces));

  i8 base_face = move->face;
  rcube_remap(rcube_mapped_copy, rcube_faces_rot_copy, base_face);

  local_const i8 band_map[6][3] = 
  {
    {band_xz,  band_yz, -1},
    {band_xz, -1, band_yz},
    {band_xz, band_yz, -1}, // TODO(antonio): do we have to do level clean up?
    {band_xz, -1, band_yz}, // TODO(antonio): do we have to do level clean up?
    {-1, band_yz, band_xz},
    {-1, band_yz, band_xz}, // TODO(antonio): do we have to do level clean up?
  };

  local_persist u8 slice[12]  = {};

  i32 band_plane = band_map[base_face][move->band_plane];
  expect((band_plane != -1) && ((band_plane == band_xz) || (band_plane == band_yz)));

  for (i32 slice_index = 0; 
       slice_index < array_count(slice);
       ++slice_index)
  {
    i32 band_index        = band_indices[band_plane][slice_index][0];
    i32 band_level_offset = move->level * band_indices[band_plane][slice_index][1];

    slice[slice_index] = (u8) (band_index + band_level_offset);
  }

  // NOTE(antonio): remember that we've remapped every face to 0 now

  // faces -> orientation
  local_const i8 rot_dir_mul[6][2] = 
  {
    {-1,  1},
    {-1,  1},
    {-1,  1},
    {-1,  1},
    {-1,  1},
    {-1,  1},
  };

  i8 rotation_sign = move->orientation == oud ? -1 : 1;
  i8 rot_dir = (i8) ((cube->cur_rotation >= 0.0f) ? ccw : cw) * rotation_sign;

  rotate_slice(rcube_mapped_copy, slice, rot_dir, move->orientation);

  if (move->level != 1)
  {
    i32 faces_map[2] = {4, 1};
    i32 face_to_rotate = faces_map[move->orientation] + ((move->orientation == oud) ? (move->level / 2) : move->level);

    if ((move->orientation == oud) && (move->level == 2))
    {
      rot_dir *= -1;
    }
    else if ((move->orientation == olr) && (move->level == 0))
    {
      rot_dir *= -1;
    }

    rcube_rotate_face(rcube_mapped_copy, face_to_rotate, rot_dir);
  }

  local_const u8 remap_face[6] = 
  {
    0, 3, 2, 1, 5, 4
  };

  rcube_remap(cube->faces, rcube_mapped_copy, remap_face[base_face]);
}

internal inline i32 rcube_index_to_face(u32 index)
{
  i32 face = index / rcube_stickers_per_face;
  return(face);
}

internal inline u32 rcube_index_to_mesh_face(u32 index)
{
  static u32 map[6] = {0, 4, 1, 5, 2, 3};
  u32 face = map[rcube_index_to_face(index)];
  return(face);
}

internal inline i32 rcube_face_normal_to_face(V4_f32 *face_normal)
{
  if (!face_normal) return -1;
  if (face_normal->x == 1.0f)
  {
    return 3;
  }
  else if (face_normal->x == -1.0f)
  {
    return 1;
  }
  else if (face_normal->y == 1.0f)
  {
    return 4;
  }
  else if (face_normal->y == -1.0f)
  {
    return 5;
  }
  else if (face_normal->z == 1.0f)
  {
    return 0;
  }
  else if (face_normal->z == -1.0f)
  {
    return 2;
  }
  else
  {
    expect_message(false, "Did not expect this face_normal");
    return -1;
  }
}

internal inline void rcube_normal_association_to_move_direction(RCube_Move_Direction *move_directions,
                                                                V4_f32               *normal,
                                                                u32                   association_index)
{
  static const V3_f32 face_to_rotation_vectors[6][2] = 
  {
    {V3( 0.0f, -1.0f,  0.0f), V3( 1.0f,  0.0f,  0.0f)}, // 0
    {V3( 0.0f, -1.0f,  0.0f), V3( 0.0f,  0.0f,  1.0f)}, // 1
    {V3( 0.0f, -1.0f,  0.0f), V3(-1.0f,  0.0f,  0.0f)}, // 2
    {V3( 0.0f, -1.0f,  0.0f), V3( 0.0f,  0.0f, -1.0f)}, // 3
    {V3( 0.0f,  0.0f,  1.0f), V3( 1.0f,  0.0f,  0.0f)}, // 4
    {V3( 0.0f,  0.0f, -1.0f), V3( 1.0f,  0.0f,  0.0f)}, // 5
  };

  static const i8 associated_faces[6][2][3] = 
  {
    { {4, -1, 5}, {1, -1, 3}, }, // 0
    { {4, -1, 5}, {2, -1, 0}, }, // 1
    { {4, -1, 5}, {3, -1, 1}, }, // 2
    { {4, -1, 5}, {0, -1, 2}, }, // 3
    { {2, -1, 0}, {1, -1, 3}, }, // 4
    { {0, -1, 2}, {1, -1, 3}, }, // 5
  };

  static const i8 face_orientation_to_band_plane[6][2] = 
  {
    {band_xz, band_yz}, {band_xz, band_xy}, {band_xz, band_yz}, {band_xz, band_xy}, {band_xy, band_yz}, {band_xy, band_yz},
  };

  i32 face = rcube_face_normal_to_face(normal);
  if (face == -1) return;

  i8 *associations = (i8 *) index_associations[association_index];
  for (u32 sticker_index = 0; sticker_index < 3; ++sticker_index)
  {
    i32 association = associations[sticker_index];
    if (association == -1)
    {
      break;
    }

    i32 association_face = association / rcube_stickers_per_face;
    if (face == association_face)
    {
      i32 remapped_association_face = association % rcube_stickers_per_face;

      move_directions[0].face            = (i8) face;
      move_directions[0].level           = (i8) (remapped_association_face / 3);
      move_directions[0].associated_face = associated_faces[face][0][move_directions[0].level];
      move_directions[0].orientation     = (i8) oud;
      move_directions[0].band_plane      = face_orientation_to_band_plane[face][oud];
      move_directions[0].rotation_vector = face_to_rotation_vectors[face][0];

      move_directions[1].face            = (i8) face;
      move_directions[1].level           = (i8) (remapped_association_face % 3);
      move_directions[1].associated_face = associated_faces[face][1][move_directions[1].level];
      move_directions[1].band_plane      = face_orientation_to_band_plane[face][olr];
      move_directions[1].orientation     = (i8) olr;
      move_directions[1].rotation_vector = face_to_rotation_vectors[face][1];
    }
  }
}

Render_Position make_rcube(Arena          *render_data,
                           RCube          *cube,
                           Matrix_f32_4x4 *rotation_mat,
                           V3_f32          translation,
                           Player_Context *player_context)
{
  u32 face_count          = 6;
  u32 cube_triangle_count = 2 * face_count;
  u32 cube_vertex_count   = (vertices_per_triangle * cube_triangle_count);

  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), cube_vertex_count * 26};

  Vertex_Buffer_Element *cube_vertices;
  Render_Position        cube_rp;

  V2_f32 initial_mouse_pos_norm = mouse_pos_normalize(player_context->initial_mouse_pos);
  V2_f32 cur_mouse_pos_norm     = mouse_pos_normalize(player_context->cur_mouse_pos);
  V2_f32 prev_mouse_pos_norm    = mouse_pos_normalize(player_context->prev_mouse_pos);

  f32     min_t        = infinity_f32;
  V4_f32 *face_normal  = NULL;
  i32     chosen_index = -1;

  local_persist RCube_Move_Direction move_directions[2];
  unused(move_directions);

  RGBA_f32 clear = rgba(0.0f, 0.0f, 0.0f, 0.0f);
  RGBA_f32 clear_face_colors[6] = {clear, clear, clear, clear, clear, clear};
  RGBA_f32 face_colors[6];

  Matrix_f32_4x4 identity = matrix4x4_identity();
  Matrix_f32_4x4 rotation = identity;

  if (player_context->choose)
  {
    rotation = matrix4x4_rotate_about_v_rodrigues(cube->rotating_about, cube->cur_rotation);
  }

  for (u32 association_index = 0;
       association_index < array_count(index_associations);
       ++association_index)
  {
    i8 *cur_associations = (i8 *) index_associations[association_index];
    copy_memory_block(face_colors, clear_face_colors, sizeof(clear_face_colors));

    b32 do_rotate = false;

    for (u32 sticker_index = 0;
         sticker_index < 3;
         ++sticker_index)
    {
      i8 association = cur_associations[sticker_index];
      if (association == -1)
      {
        break;
      }

      RGBA_f32 *association_color   = cube->color_map + cube->faces[association];
      u32       association_face    = rcube_index_to_mesh_face(association);
      face_colors[association_face] = *association_color;

      if (player_context->choose && !do_rotate)
      {
        i32 face_association = rcube_index_to_face(association);
        // i32 remapped_association = association % rcube_stickers_per_face;
        // i32 level = (cube->orientation == oud) ? (remapped_association / 3) : (remapped_association % 3);

        static b8 do_level_switch[6][3] = 
        {
          {false, false, false}, // 0
          {false, false, false}, // 1
          {false, true,  false}, // 2
          {false, false, true},  // 3
          {false, false, false}, // 4
          {false, false, true},  // 5
        };

        i8 band_plane        = move_directions[cube->orientation].band_plane;
        i8 looking_for_level =
          (i8) (do_level_switch[cube->face_rotating][band_plane] ?  2 - cube->level_rotating: cube->level_rotating);

        for (i8 band_index = 0; band_index < 12; ++band_index)
        {
          i8 base_index  = band_indices[band_plane][band_index][0];
          i8 level_mul   = band_indices[band_plane][band_index][1];
          i8 looking_for = (i8) (base_index + (looking_for_level * level_mul));

          b32 match = (association == looking_for);
          do_rotate = do_rotate || match;
        }

        do_rotate = do_rotate || (face_association == move_directions[cube->orientation].associated_face);
      }
    }

    Matrix_f32_4x4 *cube_vertex_transform = do_rotate ? &rotation : &identity;

    cube_rp       = make_cube(render_data, face_colors);
    cube_vertices = ((Vertex_Buffer_Element *) render_data->start) + cube_rp.start_pos;

    u32 iter_count = 0;

    for (u32 vertex_index = 0;
         vertex_index < cube_rp.count;
         vertex_index += 3, iter_count++)
    {
      for (u32 tri_vert_index = 0;
           tri_vert_index < 3;
           ++tri_vert_index)
      {
        Vertex_Buffer_Element *cur_vert = cube_vertices + tri_vert_index;

        V4_f32 cube_translation = V4(cube_translations[association_index], 1.0f);
        cube_translation = transform(*cube_vertex_transform, cube_translation);

        cur_vert->position = transform(*cube_vertex_transform, cur_vert->position);
        cur_vert->position._xyz = add(cur_vert->position._xyz, cube_translation._xyz);
        cur_vert->position = transform(*rotation_mat, cur_vert->position);
        cur_vert->position = add(cur_vert->position, V4(translation, 0.0f));
      }

      if (iter_count & 1)
      {
        u32 quad_start = ((iter_count - 1) * vertices_per_quad) / 2;
        Vertex_Buffer_Element *tl = &cube_vertices[quad_start];
        Vertex_Buffer_Element *tr = &cube_vertices[quad_start + 2];
        Vertex_Buffer_Element *br = &cube_vertices[quad_start + vertices_per_quad - 1];

        V4_f32 center = scale(0.5f, add(tl->position, br->position));
        V4_f32 x_axis = V4(subtract(tr->position._xyz, tl->position._xyz), 1.0f);
        V4_f32 y_axis = V4(subtract(tr->position._xyz, br->position._xyz), 1.0f);

        for (u32 quad_vertex_index = 0;
             quad_vertex_index < vertices_per_quad;
             ++quad_vertex_index)
        {
          Vertex_Buffer_Element *cur_vert = cube_vertices + quad_start + quad_vertex_index;
          cur_vert->res0 = center;
          cur_vert->res1 = x_axis;
          cur_vert->res2 = y_axis;
        }
      }

      if (player_context->dragging)
      {
        V4_f32 rotated_normal = transform(*rotation_mat, transform(*cube_vertex_transform, cube_vertices[0].normal));

        V3_f32 triangle[3] =
        {
          cube_vertices[0].position._xyz, 
          cube_vertices[1].position._xyz, 
          cube_vertices[2].position._xyz, 
        };

        V3_f32 ba    = subtract(triangle[1], triangle[0]);
        V3_f32 ca    = subtract(triangle[2], triangle[0]);
        V3_f32 tri_n = cross(ba, ca);

        f32 n_tri_n_dot = dot(rotated_normal._xyz, tri_n);

        b32 do_swap_for_winding = (n_tri_n_dot <= 0.0f);
        if (do_swap_for_winding)
        {
          swap(V3_f32, triangle[0], triangle[1]);
        }

        f32 t = infinity_f32;
        V3_f32 mouse_pos_3d = V3(initial_mouse_pos_norm.x, initial_mouse_pos_norm.y, -1.0f);

        b32 intersection_result =
          line_ray_triangle_intersect(V3(0.0f, 0.0f, 0.0f), mouse_pos_3d,
                                      triangle[0], triangle[1], triangle[2], &t);

        if (intersection_result && (t < min_t))
        {
          min_t        = t;
          face_normal  = &cube_vertices[0].normal;
          chosen_index = association_index;
        }
      }

      cube_vertices += 3;
    }
  }

  if (player_context->dragging)
  {
    if (cube->face_rotating == -1)
    {
      rcube_normal_association_to_move_direction(move_directions, face_normal, chosen_index);
      cube->face_rotating = move_directions[0].face;
    }
  }
  else
  {
    f32 from_the_nearest_quarter_turn = fmodf(absf(cube->cur_rotation), 0.25f);
    f32 end_rotation                  = 0.0f;
    f32 sign = (cube->cur_rotation < 0.0f) ? -1.0f : 1.0f;

    if (cube->face_rotating != -1)
    {

      if (from_the_nearest_quarter_turn <= 0.125f)
      {
        end_rotation = absf(cube->cur_rotation) - from_the_nearest_quarter_turn;
      }
      else
      {
        end_rotation = absf(cube->cur_rotation) + (0.25f - from_the_nearest_quarter_turn);
      }

      end_rotation *= sign;
      cube->cur_rotation = lerpf(cube->cur_rotation, 0.60f, end_rotation);
    }

    if (player_context->choose && is_between_inclusive(-0.0015f, from_the_nearest_quarter_turn, 0.0015f))
    {
      i32 move_direction_index = player_context->choose - 1;

      cube->cur_rotation = end_rotation;

      while (cube->cur_rotation != 0.0f)
      {
        rcube_do_move(cube, &move_directions[move_direction_index]);
        cube->cur_rotation = sign * (absf(cube->cur_rotation) - 0.25f);
      }

      player_context->choose = 0;
      cube->face_rotating    = -1;
      cube->cur_rotation     = 0.0f;
    }
  }

  if (player_context->dragging && (cube->face_rotating != -1))
  {
    V2_f32 _cur_mouse_v  = subtract(cur_mouse_pos_norm, initial_mouse_pos_norm);
    V2_f32 _prev_mouse_v = subtract(prev_mouse_pos_norm, initial_mouse_pos_norm);

    V3_f32 cur_mouse_v  = V3(_cur_mouse_v.x, _cur_mouse_v.y, 0.0f);

    // V3_f32 rotation_x_axis = matrix4x4_get_cols(*rotation_mat, 0)._xyz;
    // V3_f32 rotation_y_axis = matrix4x4_get_cols(*rotation_mat, 1)._xyz;

    f32 mouse_v_x_dot = cur_mouse_v.x;// dot(cur_mouse_v, rotation_x_axis);
    f32 mouse_v_y_dot = cur_mouse_v.y;// dot(cur_mouse_v, rotation_y_axis);

    const f32 inner_circle_radius = 0.002f;
    b32 cur_in  = (squared_length(_cur_mouse_v)  < inner_circle_radius);
    b32 prev_in = (squared_length(_prev_mouse_v) < inner_circle_radius);

    if (cur_in != prev_in)
    {
      if (prev_in)
      {
        player_context->choose = (absf(mouse_v_x_dot) < absf(mouse_v_y_dot)) + 1;
      }
      else
      {
        player_context->choose = 0;
      }
    }

    if (player_context->choose != 0)
    {
      i32 move_direction_index = player_context->choose - 1;
      cube->level_rotating     = move_directions[move_direction_index].level;
      cube->orientation        = move_directions[move_direction_index].orientation;
      cube->rotating_about     = move_directions[move_direction_index].rotation_vector;

      /*
      f32 rotation_sign = 1.0f;
      for (u32 vector_index = 0; vector_index < 3; ++vector_index)
      {
        f32 component = cube->rotating_about.v[vector_index];
        rotation_sign *= (component != 0.0f) ? component : 1.0f;
      }
      */

      f32 rotation_from_dot  = (player_context->choose == 1) ? mouse_v_x_dot : mouse_v_y_dot;
      cube->cur_rotation     = -1.0f * rotation_from_dot;
    }
  }

  /*
  if (absf(cube->cur_rotation) == 0.25f)
  {
    cube->cur_rotation  = 0.0f;
    cube->face_rotating = -1;
  }
  */

  return(rp);
}

void batch_make_circle_particles(Bucket_List *bucket_list,
                                          f32 min_lifetime,
                                          f32 max_lifetime,
                                          u32 min_count,
                                          u32 max_count)
{
  expect(bucket_list != NULL);
  expect(min_lifetime <= max_lifetime);
  expect(min_count <= max_count);

  u32 count_per_bucket = bucket_list_get_count_fits_in_data(bucket_list, sizeof(Circle_Particle));
  u32 count = rng_get_random_between_end_exclusive_u32(min_count, max_count + 1);

  while (count > 0)
  {
    u32 cur_count = (count > count_per_bucket) ? count_per_bucket : count;

    Bucket *cur_bucket = bucket_list_get_new_and_update(bucket_list, cur_count * sizeof(Circle_Particle));
    expect(cur_bucket != NULL);

    Circle_Particle_Header *header = (Circle_Particle_Header *) bucket_list_get_header_start(bucket_list, cur_bucket);
    header->max_lifetime = max_lifetime;

    Circle_Particle *particles = (Circle_Particle *) bucket_list_get_data_start(bucket_list, cur_bucket);

    for (u32 particle_index = 0;
         particle_index < cur_count;
         ++particle_index)
    {
      Circle_Particle *cur = particles + particle_index;

      cur->cur_color = rgba_white;
      cur->end_color = rng_get_random_rgba_f32(0.0f);
      cur->center    = V3(rng_get_random_between_f32(-1.0f, 1.0f), rng_get_random_between_f32(-1.0f, 1.0f), -2.0f);
      cur->velocity  = V3(rng_get_random_between_f32(-0.25f, 0.25f), rng_get_random_between_f32(-0.25f, 0.25f), 0.0f);
      cur->radius    = rng_get_random_between_f32(0.01f, 0.07f);
      cur->lifetime  = rng_get_random_between_f32(min_lifetime, max_lifetime);
    }

    count -= cur_count;
  }
}

Render_Position render_and_update_particles(Arena *render_data, Bucket_List **bucket_lists, u32 count)
{
  expect(count > 0);

  Arena *temp = get_temp_arena();

  f32 dt = (f32) platform_get_global_state()->dt;
  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), 0};

  Bucket_List *cur_list = bucket_lists[0];

  Bucket *prev_bucket = NULL;
  Bucket *cur_bucket  = bucket_list_get_first(cur_list);

  Pair_u32 *put_back = push_array(temp, Pair_u32, cur_list->cur_count);
  set_memory_block(put_back, (u8) -1, cur_list->cur_count * sizeof(put_back[0]));

  u32 put_back_index = 0;

  while (cur_bucket != NULL)
  {
    u32 particle_count = cur_bucket->data_size / sizeof(Circle_Particle);
    rp.count += particle_count * vertices_per_quad;

    Vertex_Buffer_Element *cur_vertex;
    cur_vertex = push_array(render_data, Vertex_Buffer_Element, particle_count * vertices_per_quad);

    Circle_Particle_Header *header    = (Circle_Particle_Header *) bucket_list_get_header_start(cur_list, cur_bucket);
    Circle_Particle        *particles = (Circle_Particle *) bucket_list_get_data_start(cur_list, cur_bucket);

    for (u32 particle_index = 0;
         particle_index < particle_count;
         ++particle_index)
    {
      Circle_Particle *cur_particle = particles + particle_index;
      V3_f32 c = cur_particle->center;
      f32    r = cur_particle->radius;

      V3_f32 tlp = V3(-r,  r, 0.0f);
      V3_f32 trp = V3( r,  r, 0.0f);
      V3_f32 blp = V3(-r, -r, 0.0f);
      V3_f32 brp = V3( r, -r, 0.0f);

      V4_f32 tl = V4(add(c, tlp), 1.0f);
      V4_f32 tr = V4(add(c, trp), 1.0f);
      V4_f32 bl = V4(add(c, blp), 1.0f);
      V4_f32 br = V4(add(c, brp), 1.0f);

      RGBA_f32 color  = wide_lerp(cur_particle->cur_color, 0.10f, cur_particle->end_color);
      V4_f32   normal = V4(c, r);

      *cur_vertex++ = vbe(tl, color, V2(0.0f, 0.0f), normal);
      *cur_vertex++ = vbe(bl, color, V2(0.0f, 0.0f), normal);
      *cur_vertex++ = vbe(tr, color, V2(0.0f, 0.0f), normal);

      *cur_vertex++ = vbe(tr, color, V2(0.0f, 0.0f), normal);
      *cur_vertex++ = vbe(bl, color, V2(0.0f, 0.0f), normal);
      *cur_vertex++ = vbe(br, color, V2(0.0f, 0.0f), normal);

      cur_particle->cur_color  = color;
      cur_particle->lifetime  -= dt;
      cur_particle->center     = add(cur_particle->center, scale(dt, cur_particle->velocity));
    }

    header->max_lifetime -= dt;
    if (header->max_lifetime < 0.0f)
    {
      put_back[put_back_index++] = {bucket_list_get_id(cur_list, prev_bucket), bucket_list_get_id(cur_list, cur_bucket)};
    }

    prev_bucket = cur_bucket;
    cur_bucket  = bucket_list_get_from_id(cur_list, cur_bucket->next_bucket_id);
  }

  bucket_list_put_back(cur_list, put_back, put_back_index);

  return(rp);
}

internal void make_sdf_shape(u8 *alpha_buffer, u32 width, u32 height, float r)
{
  V2_f32 center = V2(width / 2.0f, height / 2.0f);
  V2_f32 r_v2   = V2(r, r);

  u32 offset = 40;
  for (u32 row = offset; row < height - offset; ++row)
  {
    for (u32 col = offset; col < width - offset; ++col)
    {
      u32 buffer_pos = (row * width) + col;

      V2_f32 pos = V2((f32) col, (f32) row);

      V2_f32 pc  = subtract(pos, center);
      pc.x = absf(pc.x);
      pc.y = absf(pc.y);

      V2_f32 d2 = add(subtract(pc, center), r_v2);
      d2.x = max(d2.x, 0.0f);
      d2.y = max(d2.y, 0.0f);

      f32 value = sqrtf(squared_length(d2)) - r;
      alpha_buffer[buffer_pos] = (u8) clamp(0, (1 - value) * 255.0f, 255.0f);
    }
  }
}
