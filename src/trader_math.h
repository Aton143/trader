#ifndef TRADER_MATH_H
#include <stdlib.h>
#include <math.h>
#include "trader_base_defines.h"
#include "trader_meta.h"

internal inline f32 lerpf(f32 a, f32 t, f32 b);
internal inline f64 lerpd(f64 a, f64 t, f64 b);
internal inline V4_f32 wide_lerp(V4_f32 a, f32 t, V4_f32 b);

internal inline V2_f32 wide_clamp(V2_f32 bottom, V2_f32 v, V2_f32 top);
internal inline V4_f32 wide_clamp(V4_f32 bottom, V4_f32 v, V4_f32 top);

internal inline f32 fast_powf(f32 a, f32 b);

internal inline V2_f32 line_get_closest_point_to_point(V2_f32 p, V2_f32 line_start, V2_f32 line_end);
internal inline f32    line_get_squared_distance_from_point(V2_f32 p, V2_f32 line_start, V2_f32 line_end);

internal inline b32 approx_equal_f32(f32 a, f32 b);
internal inline f32 absf(f32 a);
internal inline f32 fmaddf(f32 m1, f32 m2, f32 a);

internal inline f32 sin01f(f32 x);
internal inline f32 cos01f(f32 x);

internal inline f32 nsin01f(f32 x);
internal inline f32 ncos01f(f32 x);

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

internal inline V2_f32 hadamard_product(V2_f32 v, V2_f32 w);

internal inline f32 dot(V2_f32 u, V2_f32 v);
internal inline f32 squared_length(V2_f32 v);

internal inline V2_f32 normalize(V2_f32 v);

internal inline V3_f32 subtract(V3_f32 a, V3_f32 b);
internal inline V3_f32 scale(f32 scale, V3_f32 v);

internal inline f32 dot(V3_f32 u, V3_f32 v);
internal inline V3_f32 normalize(V3_f32 u);
internal inline V3_f32 cross(V3_f32 u, V3_f32 v);

internal inline V3_f32 add(V3_f32 u, V3_f32 v);
internal inline V3_f32 operator +(V3_f32 a, V3_f32 b);

internal inline V3_f32 triangle_normal_ccw(f32 *vertices, u32 to_next_vertex);

internal inline f32 dot(V4_f32 u, V4_f32 v);
internal inline V4_f32 add(V4_f32 u, V4_f32 v);
internal inline V4_f32 scale(f32 scale, V4_f32 u);
internal inline V4_f32 transform(Matrix_f32_4x4 a, V4_f32 v);

internal inline V4_f32 reflect_about_xz(V4_f32 v);
internal inline V4_f32 reflect_about_xy(V4_f32 v);
internal inline V4_f32 reflect_about_yz(V4_f32 v);

#define matrix_row_count(m) (array_count((m)->rows))
#define matrix_col_count(m) (array_count((m)->row0.v))

internal inline V4_f32         matrix4x4_get_cols(Matrix_f32_4x4 matrix, u32 n);
internal inline Matrix_f32_4x4 matrix4x4_from_rows(V4_f32 row0, V4_f32 row1, V4_f32 row2, V4_f32 row3);
internal inline Matrix_f32_4x4 matrix4x4_translate(f32 x, f32 y, f32 z);
internal inline Matrix_f32_4x4 matrix4x4_diagonals(f32 row0, f32 row1, f32 row2, f32 row3);
internal inline Matrix_f32_4x4 matrix4x4_symmetric_projection(f32 near, f32 far, f32 top_bottom, f32 left_right);
internal inline Matrix_f32_4x4 matrix4x4_look_at(V3_f32 camera_pos, V3_f32 object_pos, V3_f32 helper);

internal inline Matrix_f32_4x4 matrix4x4_multiply(Matrix_f32_4x4 a, Matrix_f32_4x4 b);

// NOTE(antonio): 0 - 0 rad, 1.0f - tau rad
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_x(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_y(f32 amount);
internal inline Matrix_f32_4x4 matrix4x4_rotate_about_z(f32 amount);
internal        Matrix_f32_4x4 matrix4x4_rotate_about_v_rodrigues(V3_f32 v, f32 amount);

internal inline Matrix_f32_4x4 matrix4x4_identity(void);
internal inline Matrix_f32_4x4 add(Matrix_f32_4x4 a, Matrix_f32_4x4 b);
internal inline Matrix_f32_4x4 scale(f32 scale, Matrix_f32_4x4 a);

internal inline Rect_f32 translate(Rect_f32 rect, V2_f32 v);

internal inline b32 line_ray_triangle_intersect(V3_f32 line_segment_p0, V3_f32 line_segment_p1,
                                                V3_f32 triangle_a, V3_f32 triangle_b, V3_f32 triangle_c, f32 *t);

#define TRADER_MATH_H
#endif
