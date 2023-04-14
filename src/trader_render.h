#ifndef TRADER_RENDER_H

struct Render_Context;

extern void render_put_context(Render_Context *context);
extern Asset_Handle render_make_texture(Render_Context *context, void *texture_data, u64 width, u64 height, u64 channels);
extern Rect_f32 render_get_client_rect();

#define TRADER_RENDER_H
#endif
