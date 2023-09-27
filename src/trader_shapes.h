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

enum
{
  oud = 0,
  olr = 1,
};
typedef i8 Orientation;

enum
{
  ccw = -1,
  cw  = 1,
};
typedef i8 Rotation_Direction;

enum
{
  band_xz,
  band_yz,
  band_xy,
};

static const i8 band_indices[3][12][2] = 
{
  {{ 0,  3}, { 1,  3}, { 2,  3}, { 9,  3}, {10,  3}, {11,  3}, {18,  3}, {19,  3}, {20,  3}, {27,  3}, {28,  3}, {29,  3}},
  {{ 0,  1}, { 3,  1}, { 6,  1}, {36,  1}, {39,  1}, {42,  1}, {20, -1}, {23, -1}, {26, -1}, {45,  1}, {48,  1}, {51,  1}},
  {{ 9,  1}, {12,  1}, {15,  1}, {36,  3}, {37,  3}, {38,  3}, {29, -1}, {32, -1}, {35, -1}, {51, -3}, {52, -3}, {53, -3}},
};

struct RCube_Move_Direction
{
  i8          face;
  i8          level;
  i8          associated_face;
  i8          band_plane;
  Orientation orientation;
  V3_f32      rotation_vector;
};

struct RCube
{
  RGBA_f32    color_map[6];
  u8          faces[54]; // indices into color_map

  f32         cur_rotation;
  i32         face_rotating;
  i32         level_rotating;
  Orientation orientation;
  V3_f32      rotating_about;
};

global_const i8 index_associations[26][3] = 
{
  { 0, 42, 11},
  { 1, 43, -1},
  { 2, 44, 27},
  { 3, 14, -1},
  { 4, -1, -1},
  { 5, 30, -1},
  { 6, 17, 45},
  { 7, 46, -1},
  { 8, 47, 33},

  {10, 39, -1},
  {13, -1, -1},
  {16, 48, -1},
  {40, -1, -1},
  {41, 28, -1},
  {31, -1, -1},
  {34, 50, -1},
  {49, -1, -1},

  {18, 38, 29},
  {19, 37, -1},
  {20, 36,  9},
  {21, 32, -1},
  {22, -1, -1},
  {23, 12, -1},
  {24, 35, 53},
  {25, 52, -1},
  {26, 15, 51},
};

// NOTE(antonio): normal order
// 0 - ( 0,  0,  1)
// 1 - (-1,  0,  0)
// 2 - ( 0,  0, -1)
// 3 - ( 1,  0,  0)
// 4 - ( 0,  1,  0)
// 5 - ( 0, -1,  0)

global_const u32 cube_face_count         = 6;
global_const u32 rcube_stickers_per_face = 9;
global_const f32 rsubcube_width = 0.25f;

global_const V3_f32 rc_f = V3(0.0f, 0.0f,  rsubcube_width);
global_const V3_f32 rc_b = V3(0.0f, 0.0f, -rsubcube_width);
global_const V3_f32 rc_l = V3(-rsubcube_width, 0.0f, 0.0f);
global_const V3_f32 rc_r = V3( rsubcube_width, 0.0f, 0.0f);
global_const V3_f32 rc_u = V3(0.0f,  rsubcube_width, 0.0f);
global_const V3_f32 rc_d = V3(0.0f, -rsubcube_width, 0.0f);

global V3_f32 cube_translations[26] = 
{
  rc_f + rc_l + rc_u, 
  rc_f        + rc_u,
  rc_f + rc_r + rc_u,
  rc_f + rc_l,
  rc_f,
  rc_f + rc_r,
  rc_f + rc_l + rc_d,
  rc_f        + rc_d,
  rc_f + rc_r + rc_d,

         rc_l + rc_u, // 10
         rc_l,        // 13
         rc_l + rc_d, // 16
                rc_u, // 40
         rc_r + rc_u, // 41 
         rc_r,        // 31
         rc_r + rc_d, // 34
                rc_d, // 49

  rc_b + rc_r + rc_u, // 18
  rc_b        + rc_u, // 19
  rc_b + rc_l + rc_u, // 20
  rc_b + rc_r,        // 21
  rc_b,               // 22
  rc_b + rc_l,        // 23
  rc_b + rc_r + rc_d, // 24
  rc_b        + rc_d, // 25
  rc_b + rc_l + rc_d, // 26
};

typedef i8 Face_Color_Slot;
enum
{
  c_none  = -1,
  c_front = 0,
  c_back,
  c_top,
  c_bottom,
  c_left,
  c_right,
};

internal void put_quad(Vertex_Buffer_Element **vertices,
                       V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br,
                       RGBA_f32 color, V4_f32 normal = v4_zero);

internal void put_quad_uvs(Vertex_Buffer_Element **vertices, V2_f32 tl, V2_f32 tr, V2_f32 bl, V2_f32 br);

internal Render_Position make_cylinder(Arena *, f32, f32, f32, u32, u32);
internal Vertex_Buffer_Element *make_cylinder_along_path(Arena *, V3_f32 *, u32, f32, u32);
internal Render_Position make_player(Arena *);

internal Render_Position make_cube(Arena *render_data, RGBA_f32 *face_colors);
internal Render_Position make_rcube(Arena *render_arena, RCube *cube);

internal void batch_make_circle_particles(Bucket_List *bucket_list, f32 min_lifetime, f32 max_lifetime);
internal Render_Position render_and_update_particles(Arena *, Bucket_List **, u32);

internal void make_sdf_shape(u8 *alpha_buffer, u32 width, u32 height, float r);

#define TRADER_SHAPES_H
#endif
