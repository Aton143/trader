#ifndef TRADER_MATH_H

internal inline f32 lerpf(f32 a, f32 t, f32 b);
internal inline f64 lerpd(f64 a, f64 t, f64 b);

internal inline V4_f32 lerp(V4_f32 a, f32 t, V4_f32 b);

internal inline f32 fast_powf(f32 a, f32 b);

internal inline b32 is_point_in_rect(V2_f32 p, Rect_f32 rect);

// NOTE(antonio): now the uninteresting functions
internal inline V2_f32 V2(f32 x, f32 y);
internal inline V2_f32 add(V2_f32 a, V2_f32 b);
internal inline V2_f32 operator +(V2_f32 a, V2_f32 b);

// implemenation
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

internal inline V2_f32 operator +(V2_f32 a, V2_f32 b)
{
  return(add(a, b));
}

#endif
