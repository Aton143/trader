#ifndef TRADER_SHAPES_H

struct Circle_Particle_Header
{
  f32 max_lifetime;
};

struct Circle_Particle
{
  RGBA_f32 cur_color;
  RGBA_f32 end_color;

  V3_f32   center;
  V3_f32   velocity;

  f32      radius;
  f32      lifetime;
};

struct Render_Position
{
  u32 start_pos;
  u32 count;
};

internal void put_quad(Vertex_Buffer_Element *vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color);

internal Vertex_Buffer_Element *make_cylinder(Arena *, f32, f32, f32, u32, u32);
internal Vertex_Buffer_Element *make_cylinder_along_path(Arena *, V3_f32 *, u32, f32, u32);
internal Render_Position make_player(Arena *);

internal void batch_make_circle_particles(Bucket_List *bucket_list, f32 min_lifetime, f32 max_lifetime);
internal Render_Position render_and_update_particles(Arena *, Bucket_List **, u32);

#define TRADER_SHAPES_H
#endif
