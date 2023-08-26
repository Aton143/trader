#ifndef TRADER_SHAPES_H

internal void put_quad(Vertex_Buffer_Element *vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color);

internal Vertex_Buffer_Element *make_cylinder(Arena *, f32, f32, f32, u32, u32);
internal Vertex_Buffer_Element *make_cylinder_along_path(Arena *, V3_f32 *, u32, f32, u32);

#define TRADER_SHAPES_H
#endif
