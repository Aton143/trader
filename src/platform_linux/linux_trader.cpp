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

#include <GL/glx.h>
#include <GL/glext.h>

#include "../trader.h"

int main(int arg_count, char *arg_values[])
{
  unused(arg_count);
  unused(arg_values);

  if (!platform_common_init())
  {
    return(EXIT_FAILURE);
  }

  {
    Display *x11_display = XOpenDisplay(0);
    if (x11_display == NULL) {
      return(EXIT_FAILURE);
    }

    render_get_context()->display = x11_display;

    // NOTE(antonio): from https://github.com/Dion-Systems/4coder
#define LOAD_ATOM(x) render_get_context()->atom_##x = \
    XInternAtom(render_get_context()->display, #x, False)

    LOAD_ATOM(TARGETS);
    LOAD_ATOM(CLIPBOARD);
    LOAD_ATOM(UTF8_STRING);
    LOAD_ATOM(_NET_WM_STATE);
    LOAD_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
    LOAD_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
    LOAD_ATOM(_NET_WM_STATE_FULLSCREEN);
    LOAD_ATOM(_NET_WM_PING);
    LOAD_ATOM(_NET_WM_WINDOW_TYPE);
    LOAD_ATOM(_NET_WM_WINDOW_TYPE_NORMAL);
    LOAD_ATOM(_NET_WM_PID);
    LOAD_ATOM(WM_DELETE_WINDOW);

#undef LOAD_ATOM

    {
      i32 glx_major;
      i32 glx_minor;

      if (!glXQueryVersion(render_get_context()->display, &glx_major, &glx_minor))
      {
        return(EXIT_FAILURE);
      }

      if (!((glx_major > 1) || ((glx_major == 1) && (glx_minor >= 4))))
      {
        return(EXIT_FAILURE);
      }
    }

    GLXFBConfig glxfb_config    = {};
    XVisualInfo x11_visual_info = {};

    local_persist i32 glx_attributes[] = 
    {
      GLX_X_RENDERABLE,  True,
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE,   GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE,      8,
      GLX_GREEN_SIZE,    8,
      GLX_BLUE_SIZE,     8,
      GLX_ALPHA_SIZE,    8,
      GLX_DEPTH_SIZE,    24,
      GLX_STENCIL_SIZE,  8,
      GLX_DOUBLEBUFFER,  True,
      None
    };

    i32 configuration_count = 0;
    GLXFBConfig *configurations =
      glXChooseFBConfig(render_get_context()->display,
                        DefaultScreen(render_get_context()->display),
                        glx_attributes,
                        &configuration_count);

    if ((configurations == NULL) || (configuration_count <= 0))
    {
      return(EXIT_FAILURE);
    }

    copy_struct(&glxfb_config, configurations);
    XFree(configurations);
  }

  return(0);
}
