#ifndef TRADER_HANDLE_H

enum {
  Handle_Kind_None,
  Handle_Kind_File,
  Handle_Kind_Texture,
  Handle_Kind_Shader,
};
typedef u64 Handle_Kind;

struct Asset_Node;

struct Handle {
  Asset_Node *asset;
  u64         generation;
  Handle_Kind kind;
  utf8        id[64];
};

#define asset_data_size (kb(4) - 32 - sizeof(Handle))
#pragma pack(push, 1)
struct Asset_Node {
  Asset_Node *next;
  union {
    struct {
      u64    generation;
      u64    used;
      Handle handle;
      u64    size;
      u8     data[asset_data_size];
    };

    u8 ext_data[kb(4) - 8];
  };
};
#pragma pack(pop)

struct Asset_Pool
{
  Asset_Node *free_list_head;
};

global Asset_Pool global_asset_pool;

typedef Handle Asset_Handle;
global_const Handle nil_handle = {0, 0, Handle_Kind_None, 0};

internal b32 is_nil(Handle *handle);

internal Handle make_handle(Handle_Kind kind, utf8 *id, utf8 *location);
internal u64    handle_node_count(Handle *handle);

// implementation
internal b32 is_nil(Handle *handle)
{
  b32 result = (handle->asset      == nil_handle.asset)      &&
               (handle->generation == nil_handle.generation) &&
               (handle->kind       == nil_handle.kind);
  return(result);
}

/*
internal Handle *make_handle(Handle_Kind kind, utf8 *id)
{

}
*/

#define TRADER_HANDLE_H
#endif
