#include "../trader.h"

#undef function
#undef internal

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#define internal static

int main(int arg_count, char *arg_values[])
{
  arg_count  = 0;
  arg_values = 0;

  arg_count++;
  arg_values++;

  u8 *memory = platform_allocate_memory_pages(kb(4), NULL);
  unused(memory);

  return(0);
}
