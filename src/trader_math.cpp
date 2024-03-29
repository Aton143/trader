#include "trader_math.h"

internal inline b32 approx_equal_f32(f32 a, f32 b)
{
  b32 result = (absf(a - b) < float_epsilon);
  return(result);
}

internal inline f32 lerpf(f32 a, f32 t, f32 b)
{
  f32 interp = ((b - a) * t) + a;
  return(interp);
}

internal inline f64 lerpd(f64 a, f64 t, f64 b)
{
  f64 interp = ((b - a) * t) + a;
  return(interp);
}

internal inline V4_f32 wide_lerp(V4_f32 a, f32 t, V4_f32 b)
{
  V4_f32 res;

  res.v[0] = lerpf(a.v[0], t, b.v[0]);
  res.v[1] = lerpf(a.v[1], t, b.v[1]);
  res.v[2] = lerpf(a.v[2], t, b.v[2]);
  res.v[3] = lerpf(a.v[3], t, b.v[3]);

  return(res);
}

// from https://gist.github.com/XProger/433701300086245e0583
internal inline f32 fast_powf(f32 a, f32 b) {
	union
  {
    f32 d;
    i32 x;
  } u = {a};

	u.x = (i32) (b * (u.x - 1064866805) + 1064866805);

	return u.d;
}

internal inline f32 sin01f(f32 x)
{
  // expect(is_between_inclusive(0.0f, x, 1.0f));
  x *= tau_f32;
  f32 res = sinf(x);
  return(res);
}

internal inline f32 cos01f(f32 x)
{
  // expect(is_between_inclusive(0.0f, x, 1.0f));
  x *= tau_f32;
  f32 res = cosf(x);
  return(res);
}

internal inline f32 nsin01f(f32 x)
{
  f32 sin01 = sin01f(x);
  f32 res = (sin01 + 1.0f) * 0.5f;
  return(res);
}

internal inline f32 ns01f(f32 x)
{
  f32 cos01 = cos01f(x);
  f32 res = (cos01 + 1.0f) * 0.5f;
  return(res);
}

// internal inline f32    norm_cos01f(f32 x)

internal inline V2_f32 line_get_closest_point_to_point(V2_f32 p, V2_f32 line_start, V2_f32 line_end)
{
  V2_f32 line_vector = line_end - line_start;
  f32    t = dot(p - line_start, line_vector) / dot(line_vector, line_vector);
  t = clamp(0.0f, t, 1.0f);

  V2_f32 point_on_line = line_start + (t * line_vector);
  return(point_on_line);
}

internal inline f32 line_get_squared_distance_from_point(V2_f32 c, V2_f32 a, V2_f32 b)
{
  V2_f32 ab = b - a;
  V2_f32 ac = c - a;
  V2_f32 bc = c - b;

  f32 e = dot(ac, ab);
  if (e <= 0.0f) return dot(ac, ac);

  f32 f = dot(ab, ab);
  if (e >= f) return dot(bc, bc);

  f32 res = dot(ac, ac) - ((e * e) / f);
  res = clamp_bottom(0.0f, res);
  expect(res >= 0.0f);

  return(res);
}

internal inline b32 rect_is_point_inside(V2_f32 p, Rect_f32 rect)
{
  b32 result = is_between_inclusive(rect.x0, p.x, rect.x1) && 
               is_between_inclusive(rect.y0, p.y, rect.y1);
  return(result);
}

internal inline V2_f32 rect_get_closest_point_to_point(V2_f32 p, Rect_f32 rect)
{
  V2_f32 low_extents  = V2(rect.x0, rect.y0);
  V2_f32 high_extents = V2(rect.x1, rect.y1);

  V2_f32 closest_point = wide_clamp(low_extents, p, high_extents);
  return(closest_point);
}

struct __Closest_Side
{
  f32 dist;
  Rectangle_Side side;
};

internal int TRADER_CDECL __closest_side_comp(const void *_a, const void *_b)
{
  __Closest_Side a = *((__Closest_Side *) _a);
  __Closest_Side b = *((__Closest_Side *) _b);

  if (a.dist < b.dist) return -1;
  else                 return  1;
}

internal inline Rectangle_Side rect_get_closest_side_to_point(V2_f32         p,
                                                              Rect_f32       rect,
                                                              Rectangle_Side bias)
{
  __Closest_Side dist[4];

  // left-right
  dist[0] = {line_get_squared_distance_from_point(p, V2(rect.x0, rect.y0), V2(rect.x0, rect.y1)), rectangle_side_left};
  dist[1] = {line_get_squared_distance_from_point(p, V2(rect.x1, rect.y0), V2(rect.x1, rect.y1)), rectangle_side_right};

  // up-down
  dist[2] = {line_get_squared_distance_from_point(p, V2(rect.x0, rect.y0), V2(rect.x1, rect.y0)), rectangle_side_up};
  dist[3] = {line_get_squared_distance_from_point(p, V2(rect.x0, rect.y1), V2(rect.x1, rect.y1)), rectangle_side_down};

  qsort(dist, array_count(dist), sizeof(dist[0]), &__closest_side_comp);

  Rectangle_Side closest_side        = dist[0].side;
  if (approx_equal_f32(dist[0].dist, dist[1].dist))
  {
    Rectangle_Side second_closest_side = dist[1].side;

    if ((closest_side == bias) || (second_closest_side == bias)) {
      return bias;
    }
  }

  return(closest_side);
}

// uninteresting implementations
internal inline V2_f32 V2(f32 x, f32 y)
{
  V2_f32 res = {x, y};
  return(res);
}

internal inline V2_f32 add(V2_f32 a, V2_f32 b)
{
  V2_f32 res = {a.x + b.x, a.y + b.y};
  return(res);
}

internal inline V2_f32 subtract(V2_f32 a, V2_f32 b)
{
  V2_f32 res = {a.x - b.x, a.y - b.y};
  return(res);
}

internal inline V2_f32 scale(f32 scale, V2_f32 v)
{
  V2_f32 res = {scale * v.x, scale * v.y};
  return(res);
}

internal inline V2_f32 hadamard_product(V2_f32 v, V2_f32 w)
{
  V2_f32 res = {w.x * v.x, w.y * v.y};
  return(res);
}

internal inline f32 squared_length(V2_f32 v)
{
  f32 squared = (v.x * v.x) + (v.y * v.y);
  return(squared);
}

internal inline V2_f32 normalize(V2_f32 v)
{
  f32 factor = 1.0f / sqrtf(squared_length(v));
  V2_f32 normalized_v = scale(factor, v);
  return(normalized_v);
}

internal inline f32 dot(V2_f32 u, V2_f32 v)
{
  f32 dot_product = (u.x * v.x) + (u.y * v.y);
  return(dot_product);
}

internal inline f32 dot(V4_f32 u, V4_f32 v)
{
  f32 dot_product = (u.x * v.x) + (u.y * v.y) + (u.z * v.z) + (u.w * v.w);
  return(dot_product);
}

internal inline V2_f32 operator +(V2_f32 a, V2_f32 b)
{
  return(add(a, b));
}

internal inline V2_f32 operator -(V2_f32 a, V2_f32 b)
{
  return(subtract(a, b));
}

internal inline V2_f32 operator *(f32 s, V2_f32 v)
{
  return(scale(s, v));
}

internal inline V3_f32 V3(f32 x, f32 y, f32 z)
{
  V3_f32 res = {x, y, z};
  return(res);
}

internal inline V3_f32 subtract(V3_f32 a, V3_f32 b)
{
  V3_f32 res = V3(a.x - b.x, a.y - b.y, a.z - b.z);
  return(res);
}

internal inline V3_f32 add(V3_f32 u, V3_f32 v)
{
  V3_f32 res = V3(u.x + v.x, u.y + v.y, u.z + v.z);
  return(res);
}

internal inline V3_f32 scale(f32 scale, V3_f32 v)
{
  V3_f32 scaled = V3(scale * v.x, scale * v.y, scale * v.z);
  return(scaled);
}

internal inline f32 dot(V3_f32 u, V3_f32 v)
{
  f32 dot_product = (u.x * v.x) + (u.y * v.y) + (u.z * v.z);
  return(dot_product);
}

V3_f32 operator +(V3_f32 a, V3_f32 b)
{
  V3_f32 res = add(a, b);
  return(res);
}

internal inline V3_f32 normalize(V3_f32 u)
{
  f32 squared_length_rec = sqrtf(dot(u, u));
  expect(squared_length_rec > 0.0f);
  squared_length_rec = 1.0f / squared_length_rec;

  V3_f32 normalized = scale(squared_length_rec, u);
  return(normalized);
}

internal inline V3_f32 cross(V3_f32 u, V3_f32 v)
{
  V3_f32 cross_product = V3( (u.y * v.z) - (u.z * v.y),
                            -(u.x * v.z) + (u.z * v.x),
                             (u.x * v.y) - (u.y * v.x));
  return(cross_product);
}

internal inline V4_f32 V4(f32 x, f32 y, f32 z, f32 w)
{
  V4_f32 res = {x, y, z, w};
  return(res);
}

internal inline V4_f32 V4(V3_f32 u, f32 w)
{
  V4_f32 res = V4(u.x, u.y, u.z, w);
  return(res);
}

internal inline V4_f32 add(V4_f32 u, V4_f32 v)
{
  V4_f32 res = V4(u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w);
  return(res);
}

internal inline V4_f32 scale(f32 scale, V4_f32 u)
{
  V4_f32 res = V4(scale * u.x, scale * u.y, scale * u.z, scale * u.w);
  return(res);
}

internal inline V2_f32 wide_clamp(V2_f32 bottom, V2_f32 v, V2_f32 top)
{
  V2_f32 res =
  V2( 
    clamp(bottom.v[0], v.v[0], top.v[0]),
    clamp(bottom.v[1], v.v[1], top.v[1])
  );
  return(res);
}

internal inline V4_f32 wide_clamp(V4_f32 bottom, V4_f32 v, V4_f32 top)
{
  V4_f32 res =
  {
    clamp(bottom.v[0], v.v[0], top.v[0]),
    clamp(bottom.v[1], v.v[1], top.v[1]),
    clamp(bottom.v[2], v.v[2], top.v[2]),
    clamp(bottom.v[3], v.v[3], top.v[3]),
  };
  return(res);
}

V4_f32 matrix4x4_get_cols(Matrix_f32_4x4 matrix, u32 n)
{
  expect(is_between_inclusive(0, n, 4));
  V4_f32 result = V4(matrix.row0.v[n], matrix.row1.v[n], matrix.row2.v[n], matrix.row3.v[n]);
  return(result);
}

internal inline Matrix_f32_4x4 matrix4x4_from_rows(V4_f32 row0, V4_f32 row1, V4_f32 row2, V4_f32 row3)
{
  Matrix_f32_4x4 mat = {row0, row1, row2, row3};
  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_diagonals(f32 row0, f32 row1, f32 row2, f32 row3)
{
  Matrix_f32_4x4 mat = {};

  mat.rows[0].v[0] = row0;
  mat.rows[1].v[1] = row1;
  mat.rows[2].v[2] = row2;
  mat.rows[3].v[3] = row3;

  return(mat);
}

// NOTE(antonio): z is bounded between [0, 1]
internal inline Matrix_f32_4x4 matrix4x4_symmetric_projection(f32 near_plane, f32 far_plane, f32 top_bottom, f32 left_right)
{
  Matrix_f32_4x4 mat = {};

  expect(near_plane >= 0.0f);
  expect(far_plane > near_plane);
  expect(top_bottom > 0.0f);
  expect(left_right > 0.0f);

  f32 lr_reciprocal = 1.0f / left_right;
  f32 tb_reciprocal = 1.0f / top_bottom;
  f32 f_min_n_reciprocal = far_plane / (far_plane - near_plane);

  mat.row0 = V4(near_plane * lr_reciprocal, 0.0f, 0.0f, 0.0f);
  mat.row1 = V4(0.0f, near_plane * tb_reciprocal, 0.0f, 0.0f);
  mat.row2 = V4(0.0f,
                0.0f,
                -1.0f * f_min_n_reciprocal,
                -1.0f * near_plane * f_min_n_reciprocal);
  mat.row3 = V4(0.0f, 0.0f, -1.0f, 0.0f);

  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_translate(f32 x, f32 y, f32 z)
{
  Matrix_f32_4x4 mat = {};

  mat.row0 = V4(1.0f, 0.0f, 0.0f, x);
  mat.row1 = V4(0.0f, 1.0f, 0.0f, y);
  mat.row2 = V4(0.0f, 0.0f, 1.0f, z);
  mat.row3 = V4(0.0f, 0.0f, 0.0f, 1.0f);

  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_look_at(V3_f32 camera_pos, V3_f32 object_pos, V3_f32 helper)
{
  Matrix_f32_4x4 mat;

  V3_f32 forward = normalize(subtract(camera_pos, object_pos));
  V3_f32 right = cross(helper, forward);
  V3_f32 up = cross(forward, right);

  mat = matrix4x4_from_rows(V4(right.x,   right.y,   right.z,   0.0f),
                            V4(up.x,      up.y,      up.z,      0.0f),
                            V4(forward.x, forward.y, forward.z, 0.0f),
                            V4(0.0f,      0.0f,      0.0f,      1.0f));

  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_multiply(Matrix_f32_4x4 a, Matrix_f32_4x4 b)
{
  Matrix_f32_4x4 mat = {};

  u32 col_count = matrix_col_count(&b);
  u32 row_count = matrix_row_count(&a);

  for (u32 row = 0; row < row_count; ++row)
  {
    for (u32 col = 0; col < col_count; ++col)
    {
      V4_f32 b_col = V4(b.rows[0].v[col],
                        b.rows[1].v[col],
                        b.rows[2].v[col],
                        b.rows[3].v[col]);

      mat.rows[row].v[col] = dot(a.rows[row], b_col);
    }
  }

  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_rotate_about_x(f32 amount)
{
  f32 sin = sin01f(amount);
  f32 cos = cos01f(amount);

  Matrix_f32_4x4 res = matrix4x4_from_rows(V4(1.0f,  0.0f,  0.0f, 0.0f),
                                           V4(0.0f,  cos,  -sin,  0.0f),
                                           V4(0.0f,  sin,   cos,  0.0f),
                                           V4(0.0f,  0.0f,  0.0f, 1.0f));
  return(res);
}

internal inline Matrix_f32_4x4 matrix4x4_rotate_about_y(f32 amount)
{
  f32 to_rad = amount * tau_f32;
  f32 sin = sinf(to_rad);
  f32 cos = cosf(to_rad);

  Matrix_f32_4x4 res = matrix4x4_from_rows(V4( cos,   0.0f,  sin,  0.0f),
                                           V4( 0.0f,  1.0f,  0.0f, 0.0f),
                                           V4(-sin,   0.0f,  cos,  0.0f),
                                           V4( 0.0f,  0.0f,  0.0f, 1.0f));
  return(res);
}

internal inline Matrix_f32_4x4 matrix4x4_rotate_about_z(f32 amount)
{
  f32 to_rad = amount * tau_f32;
  f32 sin = sinf(to_rad);
  f32 cos = cosf(to_rad);

  Matrix_f32_4x4 res = matrix4x4_from_rows(V4( cos,  -sin,  0.0f, 0.0f),
                                           V4( sin,   cos,  0.0f, 0.0f),
                                           V4( 0.0f,  0.0f, 1.0f, 0.0f),
                                           V4( 0.0f,  0.0f, 0.0f, 1.0f));
  return(res);
}

internal inline Matrix_f32_4x4 matrix4x4_identity(void)
{
  Matrix_f32_4x4 mat = matrix4x4_from_rows(V4(1.0f, 0.0f, 0.0f, 0.0f),
                                           V4(0.0f, 1.0f, 0.0f, 0.0f),
                                           V4(0.0f, 0.0f, 1.0f, 0.0f),
                                           V4(0.0f, 0.0f, 0.0f, 1.0f));
  return(mat);
}

// TODO(antonio): try quaternion version?
// NOTE(antonio): https://math.stackexchange.com/questions/142821/matrix-for-rotation-around-a-vector
internal Matrix_f32_4x4 matrix4x4_rotate_about_v_rodrigues(V3_f32 v, f32 amount)
{
  Matrix_f32_4x4 mat = {};

  f32    sine  = sin01f(amount);
  f32    hsine = sin01f(amount * 0.5f);
         hsine = 2.0f * hsine * hsine;

  Matrix_f32_4x4 w = matrix4x4_from_rows(V4( 0.0f, -v.z,   v.y,  0.0f),
                                         V4( v.z,   0.0f, -v.x,  0.0f),
                                         V4(-v.y,   v.x,   0.0f, 0.0f),
                                         V4( 0.0f,  0.0f,  0.0f, 0.0f));

  mat = add(matrix4x4_identity(), scale(sine, w));
  mat = add(mat, scale(hsine, matrix4x4_multiply(w, w)));

  mat.row3.w = 1.0f;

  return(mat);
}

internal inline Matrix_f32_4x4 add(Matrix_f32_4x4 a, Matrix_f32_4x4 b)
{
  Matrix_f32_4x4 mat = {};

  for (u32 i = 0; i < 16; ++i)
  {
    mat.values[i] = a.values[i] + b.values[i];
  }

  return(mat);
}

internal inline Matrix_f32_4x4 scale(f32 scale, Matrix_f32_4x4 a)
{
  Matrix_f32_4x4 mat = {};

  for (u32 i = 0; i < 16; ++i)
  {
    mat.values[i] = scale * a.values[i];
  }

  return(mat);
}

internal inline Rect_f32 translate(Rect_f32 rect, V2_f32 v)
{
  Rect_f32 res = {};

  res.p0 = rect.p0 + v;
  res.p1 = rect.p1 + v;

  return(res);
}

internal inline V4_f32 transform(Matrix_f32_4x4 a, V4_f32 v)
{
  V4_f32 result = V4(dot(a.rows[0], v),
                     dot(a.rows[1], v),
                     dot(a.rows[2], v),
                     dot(a.rows[3], v));
  return(result);
}

internal inline f32 absf(f32 a)
{
  f32 result = (a < 0.0f) ? -a : a;
  return(result);
}

internal inline f32 fmaddf(f32 m1, f32 m2, f32 a)
{
  f32 result = fmaf(m1, m2, a);
  return(result);
}

internal inline V4_f32 reflect_about_xz(V4_f32 v)
{
  V4_f32 res = V4(v.x, -v.y, v.z, v.w);
  return(res);
}

internal inline V4_f32 reflect_about_xy(V4_f32 v)
{
  V4_f32 res = V4(v.x, v.y, -v.z, v.w);
  return(res);
}

internal inline V4_f32 reflect_about_yz(V4_f32 v)
{
  V4_f32 res = V4(-v.x, v.y, v.z, v.w);
  return(res);
}

internal inline V3_f32 triangle_normal_ccw(V3_f32 v0, V3_f32 v1, V3_f32 v2)
{
  V3_f32 in[3] = {v0, v1, v2};
  return(triangle_normal_ccw((f32 *) &in, sizeof(in[0])));
}

internal inline V3_f32 triangle_normal_ccw(f32 *vertices, u32 to_next_vertex)
{
  u8 *v_ptr = (u8 *) vertices;

  V3_f32 v0 = *((V3_f32 *) (v_ptr));
  V3_f32 v1 = *((V3_f32 *) (v_ptr + (to_next_vertex)));
  V3_f32 v2 = *((V3_f32 *) (v_ptr + (2 * to_next_vertex)));

  V3_f32 v10 = subtract(v1, v0);
  V3_f32 v20 = subtract(v2, v0);

  V3_f32 res = normalize(cross(v10, v20));
  return(res);
}

// NOTE(antonio): ripped from Real-Time Collision Detecion
b32 line_ray_triangle_intersect(V3_f32 p, V3_f32 q, V3_f32 a, V3_f32 b, V3_f32 c, f32 *intersect_t)
{
  f32 t = 0.0f;

  V3_f32 ba = subtract(b, a);
  V3_f32 ca = subtract(c, a);
  V3_f32 pq = subtract(p, q);

  V3_f32 n = cross(ba, ca);

  f32 denom = dot(pq, n);
  if (denom <= 0.0f)
  {
    return(false);
  }
  
  V3_f32 pa = subtract(p, a);
  t = dot(pa, n);

  if (t < 0.0f)
  {
    return(false);
  }

  V3_f32 e = cross(pq, pa);

  f32 v = dot(ca, e);
  if ((v < 0.0f) || (v > denom))
  {
    return(false);
  }

  f32 w = -dot(ba, e);
  if ((w < 0.0f) || ((w + v) > denom))
  {
    return(false);
  }

  if (intersect_t != NULL) 
  {
    *intersect_t = (t / denom);
  }

  return(true);
}
