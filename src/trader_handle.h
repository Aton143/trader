#ifndef TRADER_HANDLE_H

enum {
  Handle_Kind_None,

  // NOTE(antonio): these have a file associated with them
  Handle_Kind_Vertex_Shader,
  Handle_Kind_Pixel_Shader,

  Handke_Kind_Count,
};
typedef u64 Handle_Kind;

struct Asset_Node;

struct Handle {
  Asset_Node *asset;
  u64         generation;
  Handle_Kind kind;
  utf8        id[64];
};

global_const u64 asset_data_size  = kb(4) - 32 - sizeof(Handle);

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
  Arena       temp_arena;
  Asset_Node *free_list_head;
};

global_const u64 global_asset_pool_temp_arena_size = kb(512);
global Asset_Pool global_asset_pool;

typedef Handle Asset_Handle;
global_const Handle nil_handle = {0, 0, Handle_Kind_None, 0};

internal b32 is_nil(Handle *handle);

internal Handle make_handle(Handle_Kind kind, utf8 *id, utf8 *location);
internal u64    handle_node_count(Handle *handle);

#define TRADER_HANDLE_H
#endif
