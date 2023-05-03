#ifndef TRADER_HANDLE_IMPL_H
// implementation
internal b32 is_nil(Handle *handle)
{
  b32 result = (handle->asset      == nil_handle.asset)      &&
               (handle->generation == nil_handle.generation) &&
               (handle->kind       == nil_handle.kind);
  return(result);
}

internal Handle *make_handle(Handle_Kind kind, utf8 *id)
{
  Asset_Node *first = global_asset_pool.free_list_head;
  assert((first != NULL) && "no more assets for you, foolio");

  File_Buffer file_data = {};
  u64 id_length = c_string_length((char *) id);

  if (is_between_inclusive(Handle_Kind_Vertex_Shader, kind, Handle_Kind_Pixel_Shader))
  {
    zero_memory_block(win32_global_state.temp_arena.start, win32_global_state.temp_arena.used);
    win32_global_state.temp_arena.used = 0;

    file_data = platform_open_and_read_entire_file(&win32_global_state.temp_arena, id, id_length);
  }

  switch (kind)
  {
    case Handle_Kind_Pixel_Shader:
    {
      Pixel_Shader pixel_shader = render_load_pixel_shader(file_data);
      unused(pixel_shader);
    } break;
  }
  unused(id);

  return(NULL);
}
#define TRADER_HANDLE_IMPL_H
#endif
