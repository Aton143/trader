#include "trader_shapes.h"

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

  V2_f32 solid_color_uv = render_get_solid_color_uv();

  V4_f32 tl, tr, bl, br, normal;
  f32 mag = rsubcube_width / 2.0f;

  // front
  tl = V4(-mag,  mag,  mag, 1.0f);
  tr = V4( mag,  mag,  mag, 1.0f);
  bl = V4(-mag, -mag,  mag, 1.0f);
  br = V4( mag, -mag,  mag, 1.0f);
  normal = V4(0.0f, 0.0f, 1.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  // back
  tl.z = tr.z = bl.z = br.z *= -1.0f;
  normal.z *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  // top
  tl = V4(-mag,  mag, -mag, 1.0f);
  tr = V4( mag,  mag, -mag, 1.0f);
  bl = V4(-mag,  mag,  mag, 1.0f);
  br = V4( mag,  mag,  mag, 1.0f);
  normal = V4(0.0f, 1.0f, 0.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  // bottom
  tl.y = tr.y = bl.y = br.y *= -1.0f;
  normal.y *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  // left
  tl = V4(-mag,  mag, -mag, 1.0f);
  tr = V4(-mag,  mag,  mag, 1.0f);
  bl = V4(-mag, -mag, -mag, 1.0f);
  br = V4(-mag, -mag,  mag, 1.0f);
  normal = V4(-1.0f, 0.0f, 0.0f, 1.0f);

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  // right
  tl.x = tr.x = bl.x = br.x *= -1.0f;
  normal.x *= -1.0f;

  put_quad(&cur_vertex, tl, tr, bl, br, *face_colors++, normal);

  return(rp);
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

Render_Position make_rcube(Arena *render_data,
                           R_Cube *cube,
                           Matrix_f32_4x4 *rotation_mat,
                           V3_f32 translation,
                           V2_f32 mouse_pos)
{
  //mouse_pos = V2(1178.0f, 494.0f);

  u32 face_count = 6;
  u32 cube_triangle_count = 2 * face_count;
  u32 cube_vertex_count   = (vertices_per_triangle * cube_triangle_count);

  Render_Position rp = {(u32) (render_data->used / sizeof(Vertex_Buffer_Element)), cube_vertex_count * 26};

  Vertex_Buffer_Element *cube_vertices;
  Render_Position        cube_rp;

  Rect_f32 client_rect   = render_get_client_rect();
  V3_f32   mouse_pos_3d  = V3((2 * (mouse_pos.x / rect_get_width(&client_rect))) - 1.0f,
                              1.0f - (2 * (mouse_pos.y / rect_get_height(&client_rect))),
                              -1.0f);
  
  f32 min_t = infinity_f32;
  unused(min_t);

  RGBA_f32 clear = rgba(0.0f, 0.0f, 0.0f, 0.0f);
  RGBA_f32 clear_face_colors[6] = {clear, clear, clear, clear, clear, clear};
  RGBA_f32 face_colors[6];

  Matrix_f32_4x4 identity = matrix4x4_identity();
  Matrix_f32_4x4 rotation = identity;

  if (cube->face_moving != -1)
  {
    f32 rot = lerpf(cube->cur_rotation, 0.01f, -cube->rotation_direction * 0.25f);

    switch (cube->face_moving)
    {
      case 0: case 2: rotation = matrix4x4_rotate_about_z(rot); break;
      case 1: case 3: rotation = matrix4x4_rotate_about_x(rot); break;
      case 4: case 5: rotation = matrix4x4_rotate_about_y(rot); break;
    }

    cube->cur_rotation = rot;
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

      if (cube->face_moving == rcube_index_to_face(association))
      {
        do_rotate = true;
      }
    }

    Matrix_f32_4x4 *cube_vertex_transform = do_rotate ? &rotation : &identity;

    cube_rp       = make_cube(render_data, face_colors);
    cube_vertices = ((Vertex_Buffer_Element *) render_data->start) + cube_rp.start_pos;

    for (u32 vertex_index = 0;
         vertex_index < cube_rp.count;
         vertex_index += 3)
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

        cur_vert->normal._xyz = normalize(transform(*rotation_mat, cur_vert->normal)._xyz);
      }

      f32 t = infinity_f32;
      b32 intersection_result =
        line_ray_triangle_intersect(V3(0.0f, 0.0f, 0.0f), mouse_pos_3d,
                                    cube_vertices[0].position._xyz, 
                                    cube_vertices[1].position._xyz, 
                                    cube_vertices[2].position._xyz, 
                                    cube_vertices[0].normal._xyz,
                                    &t);

      if (intersection_result)
      {
        cube_vertices[0].color = rgba(0.5f, 0.5f, 0.5f, 1.0f);
        cube_vertices[1].color = rgba(0.5f, 0.5f, 0.5f, 1.0f);
        cube_vertices[2].color = rgba(0.5f, 0.5f, 0.5f, 1.0f);

        line_ray_triangle_intersect(V3(0.0f, 0.0f, 0.0f), mouse_pos_3d,
                                    cube_vertices[0].position._xyz, 
                                    cube_vertices[1].position._xyz, 
                                    cube_vertices[2].position._xyz, 
                                    cube_vertices[0].normal._xyz,
                                    NULL);
      }

      cube_vertices += 3;
    }
  }

  if (absf(cube->cur_rotation) == 0.25f)
  {
    cube->cur_rotation = 0.0f;
    cube->face_moving  = -1;
    cube->rotation_direction = 0;
  }

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
