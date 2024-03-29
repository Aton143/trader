// NOTE(antonio): Ripped straight from 4coder
// https://github.com/Dion-Systems/4coder

#define _FILE_OFFSET_BITS 64

#undef function
#undef internal

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xmd.h>
#include <X11/Xresource.h>

#define internal static

#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include "linux_opengl_defines.h"

#define GL_FUNC(N,R,P) typedef R (N##_Function)P; N##_Function *N = 0;
#include "linux_opengl_functions.h"

#include <signal.h>      // NOTE(antonio): definition of SIG* constants
#include <sys/syscall.h> // NOTE(antonio): definition of SYS_* constants
#include <fcntl.h> // NOTE(antonio): definition of SYS_* constants

#include <sys/stat.h>

#include "../trader.h"

global b32 glx_context_error = false;
global b32 global_running    = false;

internal int glx_handle_errors(Display* display, XErrorEvent* error_event)
{
  unused(display);
  unused(error_event);

  __debugbreak();
  glx_context_error = true;
  return(0);
}

internal void GLAPIENTRY gl__handle_errors(GLenum        source,
                                           GLenum        type,
                                           GLuint        id,
                                           GLenum        severity,
                                           GLsizei       length,
                                           const GLchar *message,
                                           const void   *user_param)
{
  unused(source);
  unused(id);
  unused(length);
  unused(user_param);

  __debugbreak();
  meta_log_charf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
                 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
                 type, severity, message);

  printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
         (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
         type, severity, message);
}

typedef int X11_Debug_Function(Display *display, XErrorEvent *error);
typedef X11_Debug_Function *X11_DEBUG_PROC;

internal int x11__handle_errors(Display *display, XErrorEvent *error)
{
  unused(display);
  unused(error);
  __debugbreak();
  return(0);
}

typedef int X11_IO_Debug_Function(Display *display);
typedef X11_IO_Debug_Function *X11_IO_DEBUG_PROC;

internal int x11__handle_io_errors(Display *display)
{
  unused(display);
  __debugbreak();
  return(0);
}

internal void x11_handle_events()
{
  // TODO(antonio): :( - A little better but still bad
  while (XPending(linux_platform_state.display))
  {
    XEvent event;
    XNextEvent(linux_platform_state.display, &event);

    /*
    b32 filtered = false;
    if (XFilterEvent(&event, None) == True)
    {
      filtered = true;
      if ((event.type != KeyPress) && (event.type != KeyRelease))
      {
        continue;
      }
    }
    */

    UI_Context *ui = ui_get_context();

    u64 event_id = ((u64) (event.xkey.serial << 32)) | ((u64) event.xkey.time);
    switch (event.type)
    {
      case KeyPress:
      case KeyRelease:
      {
        // NOTE(antonio): see here #include <X11/keysymdef.h>

        b32 is_key_down = (event.type == KeyPress);

        i32 key_state = event.xkey.state;

        ui_add_key_event(key_mod_event_control, ((key_state & ControlMask) != 0) && is_key_down);
        ui_add_key_event(key_mod_event_shift,   ((key_state & ShiftMask)   != 0) && is_key_down);
        ui_add_key_event(key_mod_event_alt,     ((key_state & Mod1Mask)    != 0) && is_key_down);
        ui_add_key_event(key_mod_event_super,   ((key_state & Mod4Mask)    != 0) && is_key_down);

        // TODO(antonio): why?
        event.xkey.state &= ~(ControlMask);

        // TODO(antonio): this reallllly needs to be more robust
        // some shifted chars don't work...
        // international support...
        // SO MUCH!

        Status status;
        KeySym key_sym = NoSymbol;
        utf8 buf[256] = {};
        i32 len = Xutf8LookupString(linux_platform_state.x11_input_context,
                                    &event.xkey,
                                    (char *) buf,
                                    (sizeof(buf) - 1),
                                    &key_sym,
                                    &status);

        // if (key_sym != XK_Shift_L) __debugbreak();

        if (status == XBufferOverflow)
        {
          Xutf8ResetIC(linux_platform_state.x11_input_context);
          XSetICFocus(linux_platform_state.x11_input_context);
        }

        Key_Event key = platform_convert_key_to_our_key(key_sym);
        if (key != key_event_none)
        {
          ui_add_key_event(key, is_key_down);
        }

        if (key == key_event_escape)
        {
          global_running = false;
        }
      } break;

      case ButtonPress:
      {
        Mouse_Event mouse_event = mouse_event_none;
        switch (event.xbutton.button)
        {
          case Button1:
          {
            mouse_event = mouse_event_lclick;

            // NOTE(antoniom): (inso) improves selection dragging
            // (especially in notepad-like mode).
            // we will still get mouse events when the pointer
            // leaves the window if it's dragging.

            XGrabPointer(linux_platform_state.display,
                         linux_platform_state.window_handle,
                         True, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync,
                         None, None, CurrentTime);
                        
          } break;

          case Button3:
          {
            mouse_event = mouse_event_rclick;
          } break;
        }

        ui->cur_frame_mouse_event |= mouse_event;
      } break;

      case ButtonRelease:
      {
        Mouse_Event mouse_event_to_remove = mouse_event_none;

        switch (event.xbutton.button)
        {
          case Button1:
          {
            mouse_event_to_remove = mouse_event_lclick;
            XUngrabPointer(linux_platform_state.display, CurrentTime);
          } break;

          case Button3:
          {
            mouse_event_to_remove = mouse_event_rclick;
          } break;
        }

        ui->cur_frame_mouse_event &= ~mouse_event_to_remove;
      } break;

      case ClientMessage:
      {
        Atom atom = event.xclient.data.l[0];

        // NOTE(antonio): (inso) Window X button clicked
        if (atom == linux_platform_state.atom_WM_DELETE_WINDOW)
        {
          global_running = false;
        }
        else if (atom == linux_platform_state.atom__NET_WM_PING)
        {
          // NOTE(antonio): (inso) Notify WM that we're still responding
          // (don't grey our window out)

          event.xclient.window = DefaultRootWindow(linux_platform_state.display);
          XSendEvent(linux_platform_state.display,
                     event.xclient.window,
                     False,
                     SubstructureRedirectMask | SubstructureNotifyMask,
                     &event);
        }
      } break;

      case MotionNotify:
      {
        Rect_f32 render_rect = render_get_client_rect();
        f32 x = (f32) clamp(0, event.xmotion.x, rect_get_width(&render_rect)  - 1);
        f32 y = (f32) clamp(0, event.xmotion.y, rect_get_height(&render_rect) - 1);

        ui->mouse_pos = V2(x, y);
      } break;

      case ConfigureNotify:
      {
        Rect_f32 new_rect = {0, 0, (f32) event.xconfigure.width, (f32) event.xconfigure.height};
        render_set_client_rect(new_rect);
      } break;

      case FocusIn:
      case FocusOut:
      {
        linux_platform_state.focus_event =
          (event.type == FocusIn) ?  focus_event_gain : focus_event_lose;
      } break;

      case EnterNotify:
      {
        ui->mouse_area = mouse_area_in_client;
      } break;

      case LeaveNotify:
      {
        ui->mouse_area = mouse_area_out_client;
      } break;

      default:
      {
        // TODO(antonio): keyboard refresh
      } break;
    }
  }
}

int main(int arg_count, char *arg_values[])
{
  if (!platform_common_init())
  {
    meta_log_char("Could not initialize the project\n");
    return(EXIT_FAILURE);
  }

  linux_platform_state.cur_cursor = cursor_kind_pointer;

  UI_Context            *ui     = ui_get_context();
  Common_Render_Context *render = render_get_common_context();

  unused(ui);
  unused(render);

  const Rect_f32 default_client_rect = {0, 0, 800.0f, 600.0f};
  {
    Display *x11_display = XOpenDisplay(NULL);
    if (x11_display == NULL)
    {
      meta_log_char("Could not create an X11 display\n");
      return(EXIT_FAILURE);
    }

    linux_platform_state.display = x11_display;

    XSetErrorHandler((X11_DEBUG_PROC) &x11__handle_errors);
    XSetIOErrorHandler((X11_IO_DEBUG_PROC) &x11__handle_io_errors);

    // NOTE(antonio): from https://github.com/Dion-Systems/4coder
#define LOAD_ATOM(x) linux_platform_state.atom_##x =      \
    XInternAtom(linux_platform_state.display, #x, False); \
    expect(linux_platform_state.atom_##x != None)

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
    LOAD_ATOM(ATOM_PAIR);

    LOAD_ATOM(XdndAware);
    LOAD_ATOM(XdndEnter);
    LOAD_ATOM(XdndPosition);
    LOAD_ATOM(XdndStatus);
    LOAD_ATOM(XdndActionCopy);
    LOAD_ATOM(XdndDrop);
    LOAD_ATOM(XdndFinished);
    LOAD_ATOM(XdndSelection);
    LOAD_ATOM(XdndTypeList);

#undef LOAD_ATOM

    linux_platform_state.xcontext = XUniqueContext();

    GLXFBConfig chosen_glxfb_config = {};
    XVisualInfo x11_visual_info     = {};
    {
      i32 glx_major;
      i32 glx_minor;

      if (!glXQueryVersion(linux_platform_state.display, &glx_major, &glx_minor))
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
        glXChooseFBConfig(linux_platform_state.display,
                          DefaultScreen(linux_platform_state.display),
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
        glXGetVisualFromFBConfig(linux_platform_state.display, chosen_glxfb_config);

      if (x11_visual_info_from_config == NULL)
      {
        meta_log_char("Platform does not support frame buffer format\n");
        meta_log_char("Check your OpenGL drivers are installed correctly.\n");
        return(EXIT_FAILURE);
      }

      copy_struct(&x11_visual_info, x11_visual_info_from_config);
      XFree(x11_visual_info_from_config);
    }

    render_set_client_rect(default_client_rect);

    Colormap x11_window_colormap = XCreateColormap(linux_platform_state.display,
                                                   RootWindow(linux_platform_state.display,
                                                              x11_visual_info.screen),
                                                   x11_visual_info.visual,
                                                   AllocNone);

    XSetWindowAttributes x11_window_attributes_to_set = {};

    x11_window_attributes_to_set.background_pixmap = None;
    x11_window_attributes_to_set.border_pixel      = 0;
    x11_window_attributes_to_set.backing_store     = WhenMapped;
    x11_window_attributes_to_set.event_mask        = StructureNotifyMask;
    x11_window_attributes_to_set.bit_gravity       = NorthWestGravity;
    x11_window_attributes_to_set.colormap          = x11_window_colormap;

    u32 x11_attributes_to_set_flags =
      CWBackingStore | CWBitGravity | CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    Window x11_window = XCreateWindow(linux_platform_state.display,
                                      RootWindow(linux_platform_state.display,
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

    linux_platform_state.window_handle = x11_window;

    // NOTE(antonio): (inso) window managers want the PID as a window property for some reason.
    pid_t process_id = getpid();
    XChangeProperty(linux_platform_state.display,
                    linux_platform_state.window_handle,
                    linux_platform_state.atom__NET_WM_PID,
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (u8 *) &process_id,
                    1);

    // NOTE(antonio): (inso) set the window's type to normal
    XChangeProperty(linux_platform_state.display,
                    linux_platform_state.window_handle,
                    linux_platform_state.atom__NET_WM_WINDOW_TYPE,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *) &linux_platform_state.atom__NET_WM_WINDOW_TYPE_NORMAL,
                    1);

    XStoreName(linux_platform_state.display, linux_platform_state.window_handle, "trader");

    XSizeHints *size_hints           = XAllocSizeHints();
    XWMHints   *window_manager_hints = XAllocWMHints();
    XClassHint *class_hints          = XAllocClassHint();

    {
      size_hints->flags       = PMinSize | PMaxSize | PWinGravity;

      size_hints->min_width   = 50;
      size_hints->min_height  = 50;

      size_hints->max_width   = size_hints->max_height = (1UL << 16UL);

      size_hints->win_gravity = NorthWestGravity;

      XSetWMNormalHints(linux_platform_state.display,
                        linux_platform_state.window_handle,
                        size_hints);
    }

    {
      window_manager_hints->flags         |= InputHint | StateHint;

      window_manager_hints->input          = True;
      window_manager_hints->initial_state  = NormalState;

      XSetWMHints(linux_platform_state.display,
                  linux_platform_state.window_handle,
                  window_manager_hints);
    }

    {
      class_hints->res_name  = (char *) "trader";
      class_hints->res_class = (char *) "trader";

      XSetClassHint(linux_platform_state.display,
                    linux_platform_state.window_handle,
                    class_hints);
    }

    char          *window_name_list[] = {(char *) "trader"};
    XTextProperty  window_name;
    XStringListToTextProperty(window_name_list, 1, &window_name);

    XSetWMProperties(linux_platform_state.display,
                     linux_platform_state.window_handle,
                     &window_name,
                     NULL,
                     arg_values,
                     arg_count,
                     size_hints,
                     window_manager_hints,
                     class_hints);

    XFree(window_name.value);
    XFree(size_hints);
    XFree(window_manager_hints);
    XFree(class_hints);

    {

    }

    // NOTE(antonio): (inso) make the window visible
    XMapWindow(linux_platform_state.display, linux_platform_state.window_handle);

    {
      typedef GLXContext (glXCreateContextAttribsARB_Function)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

      const char *glx_exts =
        glXQueryExtensionsString(linux_platform_state.display,
                                 DefaultScreen(linux_platform_state.display));

      glXCreateContextAttribsARB_Function *glXCreateContextAttribsARB = NULL;

#define GLXLOAD(f) f = (f##_Function*) glXGetProcAddressARB((const GLubyte*) #f);
      GLXLOAD(glXCreateContextAttribsARB);

      GLXContext glx_context = NULL;

      int (*old_handler)(Display*, XErrorEvent*) = XSetErrorHandler(&glx_handle_errors);
      unused(old_handler);

      if (glXCreateContextAttribsARB == NULL)
      {
        meta_log_char("glXCreateContextAttribsARB() not found, using old-style GLX context\n");
        glx_context = glXCreateNewContext(linux_platform_state.display, chosen_glxfb_config, GLX_RGBA_TYPE, 0, True);
      }
      else
      {
        global_const int context_attributes[] =
        {
          GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
          GLX_CONTEXT_MINOR_VERSION_ARB, 3,
          GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#if !SHIP_MODE
          GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
          GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
          None
        };

        meta_log_char("Creating GL 3.3 core context\n");
        glx_context = glXCreateContextAttribsARB(linux_platform_state.display, chosen_glxfb_config, 0, True, context_attributes);
      }

      // NOTE(antonio): sync to ensure any errors generated are processed
      XSync(linux_platform_state.display, False);

      if (glx_context_error || !glx_context)
      {
        return(EXIT_FAILURE);
      }

      XSync(linux_platform_state.display, False);

      if (!glXIsDirect(linux_platform_state.display, glx_context))
      {
        meta_log_char("Indirect GLX rendering context obtained\n");
      }
      else
      {
        meta_log_char("Direct GLX rendering context obtained\n");
      }

      // vsync??

      Bool gl_attached = glXMakeCurrent(linux_platform_state.display,
                                        linux_platform_state.window_handle,
                                        glx_context);

      if (!gl_attached)
      {
        meta_log_char("Failed to attach GL context to window\n");
        return(EXIT_FAILURE);
      }

      // NOTE(antonio): load GL functions here
#define GL_FUNC(f,R,P) GLXLOAD(f)
#include "linux_opengl_functions.h"

#define GL_FUNC(N,R,P)                                             \
      if ((N) == NULL)                                                   \
      {                                                                  \
        fprintf(stderr, "Could not load OpenGL function #N, exiting\n"); \
        return(EXIT_FAILURE);                                            \
      }
#include "linux_opengl_functions.h"

#undef GLXLOAD
    }

    XRaiseWindow(linux_platform_state.display, linux_platform_state.window_handle);

    XSync(linux_platform_state.display, False);

    Atom window_manager_protocols[] =
    {
      linux_platform_state.atom_WM_DELETE_WINDOW,
      linux_platform_state.atom__NET_WM_PING
    };

    XSetWMProtocols(linux_platform_state.display,
                                 linux_platform_state.window_handle,
                                 window_manager_protocols,
                                 array_count(window_manager_protocols));

    // NOTE(antonio): (inso) XFixes for clipboard notification
    {
      i32 xfixes_version, xfixes_error;
      Bool has_xfixes = XQueryExtension(linux_platform_state.display,
                                        "XFIXES",
                                        &xfixes_version,
                                        &linux_platform_state.xfixes_selection_event,
                                        &xfixes_error);
      linux_platform_state.has_xfixes = (has_xfixes == True);

      if (has_xfixes)
      {
        XFixesSelectSelectionInput(linux_platform_state.display,
                                   linux_platform_state.window_handle,
                                   linux_platform_state.atom_CLIPBOARD,
                                   XFixesSetSelectionOwnerNotifyMask);
      }
    } 

    // NOTE(antonio): setting locale based on ENV
    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");
    b32 supports_chosen_locale = XSupportsLocale();

    if (!supports_chosen_locale)
    {
      // NOTE(antonio): "minimum" locale
      setlocale(LC_ALL, "C");
    }

    linux_platform_state.x11_input_method =
      XOpenIM(linux_platform_state.display, NULL, NULL, NULL);

    if (!linux_platform_state.x11_input_method)
    {
      XSetLocaleModifiers("@im=none");
      linux_platform_state.x11_input_method = 
        XOpenIM(linux_platform_state.display, NULL, NULL, NULL);
    }

    if (!linux_platform_state.x11_input_method)
    {
      meta_log_char("Failed to initialize X11 inputs");
      return(EXIT_FAILURE);
    }

    XIMStyles      *obtained_styles = NULL;
    const XIMStyle  desired_style   = (XIMPreeditNothing | XIMStatusNothing);
    b32             found_style     = false;

    if (!XGetIMValues(linux_platform_state.x11_input_method,
                      XNQueryInputStyle,
                      &obtained_styles,
                      NULL) &&
        obtained_styles)
    {
      for (i32 style_index = 0;
           style_index < obtained_styles->count_styles;
           ++style_index)
      {
        XIMStyle cur_style = obtained_styles->supported_styles[style_index];
        if (cur_style == desired_style)
        {
          found_style = true;
          break;
        }
      }
    }

    if (!found_style)
    {
      meta_log_char("Failed to find supported X11 input style\n");
      return(EXIT_FAILURE);
    }

    XFree(obtained_styles);

    linux_platform_state.x11_input_context = 
      XCreateIC(linux_platform_state.x11_input_method,
                XNInputStyle,   desired_style,
                XNClientWindow, linux_platform_state.window_handle,
                XNFocusWindow,  linux_platform_state.window_handle,
                NULL);

    if (!linux_platform_state.x11_input_context)
    {
      meta_log_char("Failed to create X11 input context\n");
      return(EXIT_FAILURE);
    }

    i32   x11_input_method_event_mask;
    char *unset_values = XGetICValues(linux_platform_state.x11_input_context,
                                      XNFilterEvents,
                                      &x11_input_method_event_mask,
                                      NULL);
    if (unset_values != NULL)
    {
      x11_input_method_event_mask = 0;
    }

    u32 event_mask =
      ExposureMask         |
      KeyPressMask         |
      KeyReleaseMask       |
      ButtonPressMask      |
      ButtonReleaseMask    |
      EnterWindowMask      |
      LeaveWindowMask      |
      PointerMotionMask    |
      Button1MotionMask    |
      Button2MotionMask    |
      Button3MotionMask    |
      Button4MotionMask    |
      Button5MotionMask    |
      FocusChangeMask      |
      StructureNotifyMask  |
      ExposureMask         |
      VisibilityChangeMask |
      x11_input_method_event_mask;

    XSelectInput(linux_platform_state.display, linux_platform_state.window_handle, event_mask);

    if (!XkbQueryExtension(linux_platform_state.display,
                           0, &linux_platform_state.xkb_event, 0, 0, 0))
    {
      meta_log_char("XKB Extension not available\n");
      return(EXIT_FAILURE);
    }

    XkbSelectEvents(linux_platform_state.display,
                    XkbUseCoreKbd,
                    XkbAllEventsMask,
                    XkbAllEventsMask);

    linux_platform_state.xkb =
      XkbGetMap(linux_platform_state.display, XkbKeyTypesMask | XkbKeySymsMask, XkbUseCoreKbd);
    if (linux_platform_state.xkb == NULL)
    {
      meta_log_char("Could not get XKB keyboard map\n");
      return(EXIT_FAILURE);
    }

    if (XkbGetNames(linux_platform_state.display,
                    XkbKeyNamesMask,
                    linux_platform_state.xkb) != Success)
    {
      meta_log_char("Error getting XKB key names\n");
      return(EXIT_FAILURE);
    }

    // NOTE(antonio): (inso) closer to windows behavior 
    // (holding key doesn't generate release events)
    XkbSetDetectableAutoRepeat(linux_platform_state.display, True, NULL);

    {
      cursors[cursor_kind_pointer]._handle =
        XCreateFontCursor(linux_platform_state.display, XC_left_ptr);

      cursors[cursor_kind_finger_pointer]._handle =
        XCreateFontCursor(linux_platform_state.display, XC_hand2);

      cursors[cursor_kind_text_selection]._handle =
        XCreateFontCursor(linux_platform_state.display, XC_xterm);

      cursors[cursor_kind_left_right_direction]._handle =
        XCreateFontCursor(linux_platform_state.display, XC_sb_h_double_arrow);

      cursors[cursor_kind_up_down_direction]._handle =
        XCreateFontCursor(linux_platform_state.display, XC_sb_v_double_arrow);
    }

    // NOTE(antonio): (inso) sneaky invisible cursor
    {
      char   data = 0;
      XColor c    = {};
      Pixmap p    = XCreateBitmapFromData(linux_platform_state.display,
                                          linux_platform_state.window_handle,
                                          &data, 1, 1);

      cursors[cursor_kind_hidden]._handle =
        XCreatePixmapCursor(linux_platform_state.display, p, p, &c, &c, 0, 0);

      XFreePixmap(linux_platform_state.display, p);
    }

    {
      // NOTE(antonio): from GLFW (https://github.com/glfw/glfw)
      V2_f32 dpi = V2(96.0f, 96.0f);

      // NOTE: Basing the scale on Xft.dpi where available should provide the most
      //       consistent user experience (matches Qt, Gtk, etc), although not
      //       always the most accurate one
      char *resource_manager_str = XResourceManagerString(linux_platform_state.display);
      if (resource_manager_str != NULL)
      {
        XrmDatabase db = XrmGetStringDatabase(resource_manager_str);
        if (db)
        {
          XrmValue value;
          char *type = NULL;

          if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value))
          {
            if (type && strcmp(type, "String") == 0)
              dpi.x = dpi.y = atof(value.addr);
          }

          XrmDestroyDatabase(db);
        }
      }

      render->dpi = dpi;
    }
  }

#if !SHIP_MODE
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  if (glDebugMessageControl)
  {
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                          0, 0, false);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, 0, false);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, 0, true);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, 0, true);
  }

  if (glDebugMessageCallback)
  {
    glDebugMessageCallback(gl__handle_errors, 0);
  }
#endif

  // NOTE(antonio): OpenGL fuckery
  {
    i32 max_vertex_attributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attributes);
  }

  u32 vertex_buffer;
  u32 vertex_buffer_reader;

  {
    glGenVertexArrays(1, &vertex_buffer_reader);
    glGenBuffers(1, &vertex_buffer);

    glBindVertexArray(vertex_buffer_reader);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); 
    glBufferData(GL_ARRAY_BUFFER, render->render_data.size, NULL, GL_DYNAMIC_DRAW);
  }

  /*
   * NOTE(antonio): OpenGL NDC
   * (-1, 1) (1, 1)
   *  +--------+
   *  |        |
   *  |        |
   *  |        |
   *  |        |
   *  +--------+
   * (-1,-1) (1,-1)
   */

  /*
   * NOTE(antonio): OpenGL UVs
   * (0, 1)  (1, 1)
   *  +--------+
   *  |        |
   *  |        |
   *  |        |
   *  |        |
   *  +--------+
   * (0,0)   (1, 0)
   */

  u32 vertex_buffer_index = -1;
  {
#define REGISTER_IBE_MEMBER_FULL(member, div_type, gl_type, inc) \
    vertex_buffer_index += (inc); \
    glVertexAttribPointer(vertex_buffer_index, \
                          member_size(Instance_Buffer_Element, member) / sizeof(div_type), \
                          gl_type, GL_FALSE, sizeof(Instance_Buffer_Element), \
                          (void *) member_offset(Instance_Buffer_Element, member)); \
    glEnableVertexAttribArray(vertex_buffer_index); \
    glVertexAttribDivisor(vertex_buffer_index, 1)

#define REGISTER_IBE_MEMBER(member, div_type, gl_type) \
  REGISTER_IBE_MEMBER_FULL(member, div_type, gl_type, 1) 

    REGISTER_IBE_MEMBER(size_top_left,      f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(size_bottom_right,  f32, GL_FLOAT);

    REGISTER_IBE_MEMBER(color[0], f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(color[1], f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(color[2], f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(color[3], f32, GL_FLOAT);

    REGISTER_IBE_MEMBER(pos,                f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(corner_radius,      f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(edge_softness,      f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(border_thickness,   f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(uv_top_left,        f32, GL_FLOAT);
    REGISTER_IBE_MEMBER(uv_bottom_right,    f32, GL_FLOAT);

#undef REGISTER_IBE_MEMBER
  }

  u32 font_atlas_texture;
  {
    glGenTextures(1, &font_atlas_texture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_atlas_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    const RGBA_f32 border_color = rgba(0.0f, 0.0f, 0.0f, 0.0f);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (f32 *) &border_color);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RED, render->atlas->bitmap.width, render->atlas->bitmap.height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, render->atlas->bitmap.alpha);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  String_Const_utf8 vertex_shader_path =
    string_literal_init_type("../src/platform_linux/shader.vert", utf8);
  Handle *vertex_shader_handle = make_handle(vertex_shader_path, Handle_Kind_File);

  u32 vertex_shader;
  {
    File_Buffer temp_shader_source =
      platform_read_entire_file(get_temp_arena(),
                                vertex_shader_handle);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader,
                   1,
                   (GLchar **) &temp_shader_source.data,
                   (GLint *)   &temp_shader_source.size);
    glCompileShader(vertex_shader);

    render_debug_print_compile_errors(&vertex_shader);
  }

  String_Const_utf8 fragment_shader_path =
    string_literal_init_type("../src/platform_linux/shader.frag", utf8);
  Handle *fragment_shader_handle = make_handle(fragment_shader_path, Handle_Kind_File);

  u32 fragment_shader;
  {
    File_Buffer temp_shader_source =
      platform_read_entire_file(get_temp_arena(),
                                fragment_shader_handle);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader,
                   1,
                   (GLchar **) &temp_shader_source.data,
                   (GLint *)   &temp_shader_source.size);
    glCompileShader(fragment_shader);

    render_debug_print_compile_errors(&fragment_shader);
  }

  u32 shader_program;
  {
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    {
      utf8 *info_log = (utf8 *) stack_alloc(512);
      zero_memory_block(info_log, 512);

      i32 success;
      glGetProgramiv(shader_program, GL_LINK_STATUS, &success);

      if (!success)
      {
        glGetProgramInfoLog(shader_program, sizeof(info_log), NULL, (GLchar *) info_log);
        meta_log(info_log);
      }
    }
  }

  {
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    vertex_shader   = -1;
    fragment_shader = -1;
  }

  glUseProgram(shader_program);

  i32 texture_dimensions_location = glGetUniformLocation(shader_program, "texture_dimensions");
  i32 resolution_location         = glGetUniformLocation(shader_program, "resolution");
  i32 transform_location          = glGetUniformLocation(shader_program, "transform");

  i32 texture_sampler_location    = glGetUniformLocation(shader_program, "texture_sampler");

  {
    glUniform1i(texture_sampler_location, 0);
  }

  Matrix_f32_4x4 transform = matrix4x4_from_rows(V4(1.0f,  0.0f, 0.0f, 0.0f),
                                                 V4(0.0f, -1.0f, 0.0f, 0.0f),
                                                 V4(0.0f,  0.0f, 1.0f, 0.0f),
                                                 V4(0.0f,  0.0f, 0.0f, 1.0f));

  glEnable(GL_DEPTH_TEST);  
  glDepthFunc(GL_GREATER);
  glDepthMask(GL_TRUE);

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ZERO);

  High_Res_Time last_hpt = platform_get_high_resolution_time();

  f64 last_frame_time = 0.0f;
  b32 first_step      = true;

  f32 acc_time =  1.0f;
  f32 dir      = -1.0f;

  f32 panel_floats[16]  = {0.50f};
  u32 panel_float_index = 0;

  b32 vsync = false;
  b32 requested_vsync = true;

  f32 slider_float = 0.0f;

  GLXDrawable drawable = glXGetCurrentDrawable();

  // NOTE(antonio): query swap interval
  {
    u32 swap, max_swap;

    glXQueryDrawable(linux_platform_state.display, drawable, GLX_SWAP_INTERVAL_EXT, &swap);
    glXQueryDrawable(linux_platform_state.display, drawable, GLX_MAX_SWAP_INTERVAL_EXT,
                     &max_swap);
  }

  global_running = true;
  while (global_running)
  {
    x11_handle_events();

    if (first_step)
    {
      XConvertSelection(linux_platform_state.display,
                        linux_platform_state.atom_CLIPBOARD,
                        linux_platform_state.atom_UTF8_STRING,
                        linux_platform_state.atom_CLIPBOARD,
                        linux_platform_state.window_handle,
                        CurrentTime);
      first_step = false;
    }

    Rect_f32 render_rect = render_get_client_rect();

    ui_initialize_frame();

    panel_float_index = 0;
    ui_push_background_color(rgba_from_u8(255, 255, 255, 255));
    ui_make_panel(axis_split_vertical,
                  &panel_floats[panel_float_index++],
                  string_literal_init_type("first", utf8));

    ui_push_text_color(1.0f, 1.0f, 1.0f, 1.0f);

    ui_do_text_edit(&debug_teb, "text editor");
    ui_do_formatted_string("Last frame time: %2.6fs", last_frame_time);
    ui_do_formatted_string("Mouse position: (%.0f, %.0f)",
                           (double) ui->mouse_pos.x, (double) ui->mouse_pos.y);

    if (ui->mouse_area == mouse_area_in_client)
    {
      ui_do_string(string_literal_init_type("Mouse is in client", utf8));
    }
    else if (ui->mouse_area == mouse_area_out_client)
    {
      ui_do_string(string_literal_init_type("Mouse is not in client", utf8));
    }
    else
    {
      ui_do_string(string_literal_init_type("I don't where the hell the mouse is", utf8));
    }

    ui_do_formatted_string("Slider float: %.4f", (double) slider_float);
    ui_do_slider_f32(string_literal_init_type("slider", utf8), &slider_float, 0.0f, 1.0f);

    ui_prepare_render_from_panels(ui_get_sentinel_panel(), render_rect);
    ui_flatten_draw_layers();

    Constant_Buffer constant_buffer_items = {};
    {
      constant_buffer_items.atlas_width   = (f32) render->atlas->bitmap.width;
      constant_buffer_items.atlas_height  = (f32) render->atlas->bitmap.height;

      constant_buffer_items.client_width  = rect_get_width(&render_rect);
      constant_buffer_items.client_height = rect_get_height(&render_rect);
    }

    glUniform4f(texture_dimensions_location,
                constant_buffer_items.atlas_width,
                constant_buffer_items.atlas_height,
                0.0f, 0.0f);
    glUniform2f(resolution_location,
                constant_buffer_items.client_width,
                constant_buffer_items.client_height);
    glUniformMatrix4fv(transform_location, 1, GL_TRUE, transform.values);

    {
      glViewport(0, 0, (i32) rect_get_width(&render_rect), (i32) rect_get_height(&render_rect));

      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClearDepth(0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_atlas_texture);

      {
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBindVertexArray(vertex_buffer_reader);

        void *mapped_data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        if (mapped_data != NULL)
        {
          copy_memory_block(mapped_data, render->render_data.start, render->render_data.used);
        }
        else
        {
          meta_log_char("Failed to map UI instance buffer\n");
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
      }

      u32 instance_count = render->render_data.used / sizeof(Instance_Buffer_Element);

      for (u32 draw_layer_index = 0;
           draw_layer_index < array_count(ui->render_layers);
           ++draw_layer_index)
      {
        u32 instance_count =
          (u32) (ui->render_layers[draw_layer_index].used / sizeof(Instance_Buffer_Element));
        glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP,
                                          0,
                                          4,
                                          instance_count,
                                          ui->flattened_draw_layer_indices[draw_layer_index]);
      }
    }

    if (vsync != requested_vsync)
    {
      requested_vsync = !!requested_vsync;

      if (glXSwapIntervalEXT != NULL)
      {
        glXSwapIntervalEXT(linux_platform_state.display, drawable, requested_vsync);
      }
      else if (glXSwapIntervalMESA != NULL)
      {
        i32 glx_return = glXSwapIntervalMESA(requested_vsync);
        expect(glx_return != GLX_BAD_CONTEXT);
      }
      else if (glXSwapIntervalSGI != NULL)
      {
        i32 glx_return = glXSwapIntervalSGI(requested_vsync);
        expect((glx_return != GLX_BAD_CONTEXT) && (glx_return != GLX_BAD_VALUE));
      }

      vsync = requested_vsync;
    }

    glXSwapBuffers(linux_platform_state.display, linux_platform_state.window_handle);

    {
      arena_reset(&render->render_data);
      arena_reset(&render->triangle_render_data);

      ui->mouse_delta            = {0.0f, 0.0f};
      ui->mouse_wheel_delta      = {0.0f, 0.0f};
      ui->prev_frame_mouse_event = ui->cur_frame_mouse_event;

      for (u32 interaction_index = 0;
           interaction_index < array_count(ui->interactions);
           ++interaction_index) 
      {
        UI_Interaction *cur_int = &ui->interactions[interaction_index];
        cur_int->frames_left--;

        if (cur_int->frames_left < 0)
        {
          ui->interactions[interaction_index] = {};
        }
      }

      if (linux_platform_state.desired_cursor != cursor_kind_none)
      {
        if (linux_platform_state.desired_cursor != linux_platform_state.cur_cursor)
        {
          XDefineCursor(linux_platform_state.display,
                        DefaultRootWindow(linux_platform_state.display),
                        cursors[linux_platform_state.desired_cursor]._handle);
          XFlush(linux_platform_state.display);

          linux_platform_state.cur_cursor = linux_platform_state.desired_cursor;
        }
      }
      else if (linux_platform_state.cur_cursor != cursor_kind_pointer)
      {
        XDefineCursor(linux_platform_state.display,
                      DefaultRootWindow(linux_platform_state.display),
                      cursors[cursor_kind_pointer]._handle);
        XFlush(linux_platform_state.display);

          linux_platform_state.cur_cursor = cursor_kind_pointer;
      }

      linux_platform_state.focus_event    = focus_event_none;
      linux_platform_state.desired_cursor = cursor_kind_none;

      acc_time += dir * (1.0f / 60.f);
      if (acc_time > 1.0f)
      {
        acc_time =  1.0f;
        dir      = -1.0f;
      }
      else if (acc_time < 0.0f)
      {
        acc_time = 0.0f;
        dir      = 1.0f;
      }
    }

    // meta_collate_timing_records();

    {
      High_Res_Time cur_hpt = platform_get_high_resolution_time();
      last_frame_time =
        platform_high_resolution_time_to_seconds(platform_hrt_subtract(cur_hpt, last_hpt));
      last_hpt = cur_hpt;
    }
  }

  return(0);
}
