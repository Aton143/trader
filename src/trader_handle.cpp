#include "trader_handle.h"

b32 is_nil(Handle *handle)
{
  b32 result = (handle->generation == nil_handle.generation) &&
               (handle->flags      == nil_handle.flags);

  return(result);
}

void handle_make_nil(Handle *handle)
{
  copy_struct(handle, (Handle *) &nil_handle);
}

void asset_node_put_back(Asset_Node *node)
{
  expect(node != NULL);
  node->next = global_asset_pool.free_list_head;
  global_asset_pool.free_list_head = node;
}

Handle *handle_make(String_Const_utf8 id, Handle_Flag flags, Handle *previous_handle, Thread_Context *thread_context)
{
  Handle *result = NULL;

  Asset_Node *first = global_asset_pool.free_list_head;
  expect_message(first != NULL, "no more assets for you, foolio");

  if (previous_handle == NULL)
  {
    if (flags & handle_flag_file)
    {
      expect(flags & handle_flag_file);

      result = &first->handle;
      zero_struct(result);
      result->flags = flags;

      if (platform_open_file(id.str, id.size, result, thread_context))
      {
        String_Const_utf8 file_name = platform_get_file_name_from_path(&id);

        result->id    = file_name;
        result->flags = flags;

        global_asset_pool.free_list_head = global_asset_pool.free_list_head->next;
        first->next = NULL;
      }
      else
      {
        // TODO(antonio): put back to free list atomically
        asset_node_put_back(first);

        handle_make_nil(result);
      }
    }
  }
  else
  {
    expect(previous_handle->flags == flags);
    expect(!compare_memory_block(previous_handle->id.str, id.str,
                                 min(id.size, previous_handle->id.size)));

    result = previous_handle;
  }

  return(result);
}
