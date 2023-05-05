#ifndef TRADER_HANDLE_IMPL_H
// implementation
internal b32 is_nil(Handle *handle)
{
  b32 result = (handle->generation == nil_handle.generation) &&
               (handle->kind       == nil_handle.kind);

  return(result);
}

internal Handle *make_handle(utf8 *id, Handle_Kind kind, Handle *previous_handle)
{
  Handle *result = NULL;

  Asset_Node *first = global_asset_pool.free_list_head;
  assert((first != NULL) && "no more assets for you, foolio");

  u64 id_size = c_string_length(id);
  if (previous_handle == NULL)
  {
    if (is_between_inclusive(Handle_Kind_File, kind, Handle_Kind_File))
    {
      result = &first->handle;
      zero_struct(result);

      if (platform_open_file(id, id_size, result))
      {
        copy_memory_block(result->id, id, min(id_size, array_count(result->id)));
        result->kind = kind;

        global_asset_pool.free_list_head = global_asset_pool.free_list_head->next;
        first->next = NULL;
      }
    }
  }
  else
  {
    assert(previous_handle->kind == kind);
    assert(!copy_memory_block(previous_handle->id, id,
                              min(id_size, array_count(result->id))));

    result = previous_handle;
  }

  return(result);
}
#define TRADER_HANDLE_IMPL_H
#endif
