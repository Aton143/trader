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

  common_render->default_font =
    platform_open_and_read_entire_file(global_arena,
                                       default_font_path.str,
                                       default_font_path.size);
  expect(common_render->default_font.used != 0);

  render_atlas_initialize(global_arena,
                          common_render->atlas,
                          &common_render->default_font,
                          (f32 *) default_font_heights,
                          array_count(default_font_heights),
                          512, 512);

  Player_Context *player_context = player_get_context();

  player_context->rotation_max_speed = 0.008f;
  player_context->lerp_factor = 0.10f;

  return(result);
}

internal void platform_thread_init(void)
{
  for (u32 thread_context_index = 0;
       thread_context_index < thread_count;
       ++thread_context_index)
  {
    thread_contexts[thread_context_index].local_temp_arena.arena = arena_alloc(global_temp_arena_size, 1, NULL);
  }
}

internal void platform_debug_printf(char *format, ...)
{
  char buffer[512] = {};
  va_list args;
  va_start(args, format);

  stbsp_vsnprintf(buffer, array_count(buffer), format, args);
  platform_debug_print(buffer);

  va_end(args);
}
