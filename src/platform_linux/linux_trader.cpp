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
    meta_log_char("Could not initialize the project\n");
    return(EXIT_FAILURE);
  }

  {
    Display *x11_display = XOpenDisplay(0);
    if (x11_display == NULL)
    {
    meta_log_char("Could not create an X11 display\n");
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

    GLXFBConfig chosen_glxfb_config = {};
    XVisualInfo x11_visual_info     = {};
    {
      i32 glx_major;
      i32 glx_minor;

      if (!glXQueryVersion(render_get_context()->display, &glx_major, &glx_minor))
      {
        meta_log_char("Failed to query GL version\n");
        return(EXIT_FAILURE);
      }

      if (!((glx_major > 1) || ((glx_major == 1) && (glx_minor >= 4))))
      {
        meta_log_charf("Bad GLX Version: Given GL major version %d, minor version %d "
                       "- Should be at least GL major version 1, minor version 4\n",
                       glx_major, glx_minor);
        return(EXIT_FAILURE);
      }

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
        meta_log_char("Could not find a configuration for OpenGL\n");
        return(EXIT_FAILURE);
      }

      copy_struct(&chosen_glxfb_config, configurations);
      XFree(configurations);

      XVisualInfo *x11_visual_info_from_config =
        glXGetVisualFromFBConfig(render_get_context()->display, chosen_glxfb_config);

      if (x11_visual_info_from_config == NULL)
      {
        meta_log_char("Platform does not support frame buffer format\n");
        meta_log_char("Check your OpenGL drivers are installed correctly.\n");
        return(EXIT_FAILURE);
      }

      copy_struct(&x11_visual_info, x11_visual_info_from_config);
      XFree(x11_visual_info_from_config);
    }

    const Rect_f32 default_client_rect = {0, 0, 800.0f, 600.0f};
    render_set_client_rect(default_client_rect);

    Colormap x11_window_colormap = XCreateColormap(render_get_context()->display,
                                                   RootWindow(render_get_context()->display,
                                                              x11_visual_info.screen),
                                                   x11_visual_info.visual,
                                                   AllocNone);

    XSetWindowAttributes x11_window_attributes_to_set = {};

    x11_window_attributes_to_set.backing_store = WhenMapped;
    x11_window_attributes_to_set.event_mask    = StructureNotifyMask;
    x11_window_attributes_to_set.bit_gravity   = NorthWestGravity;
    x11_window_attributes_to_set.colormap      = x11_window_colormap;

    u32 x11_attributes_to_set_flags =
      CWBackingStore | CWBitGravity | CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    Window x11_window = XCreateWindow(render_get_context()->display,
                                      RootWindow(render_get_context()->display,
                                                 x11_visual_info.screen),
                                      0,
                                      0,
                                      (i32) rect_get_width(&default_client_rect),
                                      (i32) rect_get_height(&default_client_rect),
                                      0,
                                      x11_visual_info.depth,
                                      InputOutput,
                                      x11_visual_info.visual,
                                      x11_attributes_to_set_flags,
                                      &x11_window_attributes_to_set);

    if (x11_window == NULL)
    {
      meta_log_char("XCreateWindow failed. Make sure your display is set up correctly.\n");
      return(EXIT_FAILURE);
    }
  }

  return(0);
}
