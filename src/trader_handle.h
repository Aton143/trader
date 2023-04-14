#ifndef TRADER_HANDLE_H

enum {
  Handle_Kind_None,
  Handle_Kind_File,
  Handle_Kind_Texture,
};
typedef u64 Handle_Kind;

struct Handle {
  u64         index;
  u64         generation;
  Handle_Kind kind;
};

typedef Handle Asset_Handle;
global_const Handle nil_handle = {0, 0, Handle_Kind_None};

#define TRADER_HANDLE_H
#endif
