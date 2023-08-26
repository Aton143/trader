#ifndef TRADER_SHAPES_H

internal void put_quad(Vertex_Buffer_Element *vertices, V4_f32 tl, V4_f32 tr, V4_f32 bl, V4_f32 br, RGBA_f32 color);

internal Vertex_Buffer_Element *make_cylinder(Arena *render_arena,
                                              f32    base_radius,
                                              f32    top_radius,
                                              f32    height,
                                              u32    sector_count,
                                              u32    stack_count);

#define TRADER_SHAPES_H
#endif
