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
  unused(arg_count);
  unused(arg_values);

  if (!platform_common_init())
  {
    return(EXIT_FAILURE);
  }

  return(0);
}
