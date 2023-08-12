#include "trader_platform.h"

internal b32 platform_common_init(void)
{
  b32 result = true;

  rng_init();

  platform_thread_init();
  rng_init();

  for (u32 thread_context_index = 0;
       thread_context_index < thread_count;
       ++thread_context_index)
  {
    thread_contexts[thread_context_index].local_temp_arena.arena = arena_alloc(global_temp_arena_size, 1, NULL);
  }

#if !SHIP_MODE
  meta_init();
#endif

  Arena *global_arena = platform_get_global_arena();
  *global_arena = arena_alloc(global_memory_size, 4, (void *) global_memory_start_addr);

  Arena render_data               = arena_alloc(render_data_size, 1, NULL);
  Arena triangle_render_data      = arena_alloc(triangle_render_data_size, 1, NULL);

  Asset_Node *asset_pool_start = (Asset_Node *) arena_push(global_arena, asset_pool_size);
  u64 asset_count = asset_pool_size / sizeof(*asset_pool_start);
  for (u64 asset_index = 0;
       asset_index < asset_count - 1;
       ++asset_index)
  {
    asset_pool_start[asset_index].next = &asset_pool_start[asset_index + 1];
  }
  global_asset_pool.free_list_head = asset_pool_start;

  ui_initialize(ui_get_context());

  // f32 default_font_heights[] = {24.0f};

  Common_Render_Context *common_render = render_get_common_context();
  common_render->atlas = push_struct_zero(global_arena, Texture_Atlas);

  // TODO(antonio): font data

  common_render->render_data          = render_data;
  common_render->triangle_render_data = triangle_render_data;

  return(result);
}
