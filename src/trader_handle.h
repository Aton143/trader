#ifndef TRADER_HANDLE_H

#include <stdio.h>
#include "trader_base_defines.h"
#include "trader_memory.h"

#if OS_LINUX
struct OS_Handle
{
  i32 __handle;
};
#endif

enum {
  Handle_Kind_None,
  Handle_Kind_File,
  Handle_kind_Count,
};
typedef u64 Handle_Kind;

struct Asset_Node;
struct OS_Handle;

struct Handle {
  u64               generation;
  Handle_Kind       kind;
  String_Const_utf8 id;

  union
  {
    OS_Handle file_handle;
  };
};

global_const u64 asset_pool_size          = mb(1);
global_const u64 asset_first_element_size = kb(4) - 32 - sizeof(Handle);

struct Asset_Node {
  Asset_Node *next;
  Handle      handle;

#if 0
  union {
    struct {
      u64    used;
      u64    size;
      u8     data[asset_first_element_size];
    };

    u8 ext_data[kb(4) - 8];
  };
#endif
};

struct Asset_Pool
{
  Asset_Node *free_list_head;
};

global Asset_Pool global_asset_pool;

typedef Handle Asset_Handle;
global_const Handle nil_handle = {};

internal b32  is_nil(Handle *handle);
internal void make_nil(Handle *handle);

internal Handle *make_handle(String_Const_utf8  id,
                             Handle_Kind        kind,
                             Handle            *previous_handle = NULL);
internal u64     handle_node_count(Handle *handle);

#define TRADER_HANDLE_H
#endif
