#ifndef TRADER_MATH_H

internal f32 lerpf(f32 a, f32 t, f32 b);
internal f64 lerpd(f64 a, f64 t, f64 b);

internal f32 powf_fast(f32 a, f32 b);

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

// from https://gist.github.com/XProger/433701300086245e0583
internal f32 powf_fast(f32 a, f32 b) {
	union
  {
    f32 d;
    i32 x;
  } u = {a};

	u.x = (i32) (b * (u.x - 1064866805) + 1064866805);

	return u.d;
}

#endif
