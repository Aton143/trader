#include "trader_handle.h"

internal b32 is_nil(Handle *handle)
{
  b32 result = (handle->generation == nil_handle.generation) &&
               (handle->kind       == nil_handle.kind);

  return(result);
}

internal void make_nil(Handle *handle)
{
  copy_struct(handle, (Handle *) &nil_handle);
}

internal Handle *make_handle(String_Const_utf8 id, Handle_Kind kind, Handle *previous_handle)
{
  Handle *result = NULL;

  Asset_Node *first = global_asset_pool.free_list_head;
  expect_message(first != NULL, "no more assets for you, foolio");

  if (previous_handle == NULL)
  {
    if (is_between_inclusive(Handle_Kind_File, kind, Handle_Kind_File))
    {
      result = &first->handle;
      zero_struct(result);

      if (platform_open_file(id.str, id.size, result))
      {
        String_Const_utf8 file_name = platform_get_file_name_from_path(&id);

        result->id   = id;
        result->kind = kind;

        global_asset_pool.free_list_head = global_asset_pool.free_list_head->next;
        first->next = NULL;
      }
    }
  }
  else
  {
    expect(previous_handle->kind == kind);
    expect(!compare_memory_block(previous_handle->id.str, id.str,
                                 min(id.size, previous_handle->id.size)));

    result = previous_handle;
  }

  return(result);
}
