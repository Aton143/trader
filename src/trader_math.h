#ifndef TRADER_MATH_H
#include <math.h>

internal inline f32 lerpf(f32 a, f32 t, f32 b);
internal inline f64 lerpd(f64 a, f64 t, f64 b);
internal inline V4_f32 wide_lerp(V4_f32 a, f32 t, V4_f32 b);

internal inline V2_f32 wide_clamp(V2_f32 bottom, V2_f32 v, V2_f32 top);
internal inline V4_f32 wide_clamp(V4_f32 bottom, V4_f32 v, V4_f32 top);

internal inline f32 fast_powf(f32 a, f32 b);

internal inline V2_f32 line_get_closest_point_to_point(V2_f32 p, V2_f32 line_start, V2_f32 line_end);
internal inline f32    line_get_squared_distance_from_point(V2_f32 p, V2_f32 line_start, V2_f32 line_end);

typedef u32 Rectangle_Side;
enum
{
  rectangle_side_none,
  rectangle_side_up,
  rectangle_side_right,
  rectangle_side_down,
  rectangle_side_left,
};

internal inline b32            rect_is_point_inside(V2_f32 p, Rect_f32 rect);
internal inline V2_f32         rect_get_closest_point_to_point(V2_f32 p, Rect_f32 rect);
internal inline Rectangle_Side rect_get_closest_side_to_point(V2_f32 p, Rect_f32 rect, Rectangle_Side bias);

// NOTE(antonio): now the uninteresting functions
internal inline V2_f32 add(V2_f32 a, V2_f32 b);
internal inline V2_f32 operator +(V2_f32 a, V2_f32 b);

internal inline V2_f32 subtract(V2_f32 a, V2_f32 b);
internal inline V2_f32 operator -(V2_f32 a, V2_f32 b);

internal inline V2_f32 scale(f32 scale, V2_f32 v);
internal inline V2_f32 operator *(f32 scale, V2_f32 v);

internal inline f32 dot(V2_f32 u, V2_f32 v);
internal inline f32 squared_length(V2_f32 v);

internal inline V2_f32 normalize(V2_f32 v);

internal inline Matrix_f32_4x4 matrix4x4_from_rows(V4_f32 row0, V4_f32 row1, V4_f32 row2, V4_f32 row3);

// NOTE(antonio): 0 - 0 rad, 1.0f - tau rad
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_x(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_y(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_z(f32 amount);

internal inline Rect_f32 translate(Rect_f32 rect, V2_f32 v);

// implementation
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
internal f32 fast_powf(f32 a, f32 b) {
	union
  {
    f32 d;
    i32 x;
  } u = {a};

	u.x = (i32) (b * (u.x - 1064866805) + 1064866805);

	return u.d;
}

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
  if (approx_equal(dist[0].dist, dist[1].dist))
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

internal inline V4_f32 V4(f32 x, f32 y, f32 z, f32 w)
{
  V4_f32 res = {x, y, z, w};
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

internal inline Matrix_f32_4x4 matrix4x4_from_rows(V4_f32 row0, V4_f32 row1, V4_f32 row2, V4_f32 row3)
{
  Matrix_f32_4x4 mat = {row0, row1, row2, row3};
  return(mat);
}

internal inline Matrix_f32_4x4 matrix4x4_rotate_about_x(f32 amount)
{
  f32 to_rad = amount * tau_f32;
  f32 sin = sinf(to_rad);
  f32 cos = cosf(to_rad);

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

internal inline Rect_f32 translate(Rect_f32 rect, V2_f32 v)
{
  Rect_f32 res = {};

  res.p0 = rect.p0 + v;
  res.p1 = rect.p1 + v;

  return(res);
}

#endif
