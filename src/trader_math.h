#ifndef TRADER_MATH_H
#include <math.h>

internal inline f32 lerpf(f32 a, f32 t, f32 b);
internal inline f64 lerpd(f64 a, f64 t, f64 b);

internal inline V4_f32 lerp(V4_f32 a, f32 t, V4_f32 b);

internal inline f32 fast_powf(f32 a, f32 b);

internal inline b32 is_point_in_rect(V2_f32 p, Rect_f32 rect);

// NOTE(antonio): now the uninteresting functions
internal inline V2_f32 V2(f32 x, f32 y);

internal inline V2_f32 add(V2_f32 a, V2_f32 b);
internal inline V2_f32 operator +(V2_f32 a, V2_f32 b);

internal inline V2_f32 subtract(V2_f32 a, V2_f32 b);
internal inline V2_f32 operator -(V2_f32 a, V2_f32 b);

internal inline V2_f32 scale(f32 scale, V2_f32 v);
internal inline V2_f32 operator *(f32 scale, V2_f32 v);

internal inline V2_f32 normalize(V2_f32 v);

internal inline V4_f32 V4(f32 x, f32 y, f32 z, f32 w = 1.0f);
internal inline V4_f32 wide_clamp(V4_f32 bottom, V4_f32 v, V4_f32 top);

internal inline Matrix_f32_4x4 matrix4x4_from_rows(V4_f32 row0, V4_f32 row1, V4_f32 row2, V4_f32 row3);

// NOTE(antonio): 0 - 0 rad, 1.0f - tau rad
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_x(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_y(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_z(f32 amount);

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

internal inline V4_f32 lerp(V4_f32 a, f32 t, V4_f32 b)
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

internal inline b32 is_point_in_rect(V2_f32 p, Rect_f32 rect)
{
  b32 result = is_between_inclusive(rect.x0, p.x, rect.x1) && 
               is_between_inclusive(rect.y0, p.y, rect.y1);
  return(result);
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


internal inline V2_f32 normalize(V2_f32 v)
{
  f32 factor = 1.0f / sqrtf((v.x * v.x) + (v.y * v.y));
  V2_f32 normalized_v = scale(factor, v);
  return(normalized_v);
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

internal inline V4_f32 V4(f32 x, f32 y, f32 z, f32 w)
{
  V4_f32 res = {x, y, z, w};
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

#endif
