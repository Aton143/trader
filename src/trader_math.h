#ifndef TRADER_MATH_H

internal f32 lerpf(f32 a, f32 t, f32 b);
internal f64 lerpd(f64 a, f64 t, f64 b);

internal V4_f32 lerp(V4_f32 a, f32 t, V4_f32 b);

internal f32 fast_powf(f32 a, f32 b);

// implemenation
internal f32 lerpf(f32 a, f32 t, f32 b)
{
  f32 interp = ((b - a) * t) + a;
  return(interp);
}

internal f64 lerpd(f64 a, f64 t, f64 b)
{
  f64 interp = ((b - a) * t) + a;
  return(interp);
}

internal V4_f32 lerp(V4_f32 a, f32 t, V4_f32 b)
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

#endif
