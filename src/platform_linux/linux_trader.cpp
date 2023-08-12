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

internal void x11_handle_events()
{
  // TODO(antonio): :(
  while (XPending(linux_platform_state.display))
  {
    XEvent event;
    XNextEvent(linux_platform_state.display, &event);

    b32 filtered = false;
    if (XFilterEvent(&event, None) == True)
    {
      filtered = true;
      if ((event.type != KeyPress) && (event.type != KeyRelease))
      {
        continue;
      }
    }

    u64 event_id = ((u64) (event.xkey.serial << 32)) | ((u64) event.xkey.time);
    switch (event.type)
    {
      case KeyPress:
      {
        // __debugbreak();
      } break;

      case ClientMessage:
      {
        // __debugbreak();
        Atom atom = event.xclient.data.l[0];

        // NOTE(antonio): (inso) Window X button clicked
        if (atom == linux_platform_state.atom_WM_DELETE_WINDOW)
        {
          global_running = false;
        }
        else if (atom == linux_platform_state.atom__NET_WM_PING)
        {
          // Notify WM that we're still responding (don't grey our window out).
          event.xclient.window = DefaultRootWindow(linux_platform_state.display);
          XSendEvent(linux_platform_state.display,
                     event.xclient.window,
                     False,
                     SubstructureRedirectMask | SubstructureNotifyMask,
                     &event);
        }
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

    // NOTE(antonio): from https://github.com/Dion-Systems/4coder
#define LOAD_ATOM(x) linux_platform_state.atom_##x = \
    XInternAtom(linux_platform_state.display, #x, False)

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

    // NOTE(antonio): (inso) set the window's type to normal
    XChangeProperty(linux_platform_state.display,
                    linux_platform_state.window_handle,
                    linux_platform_state.atom__NET_WM_WINDOW_TYPE,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *) &linux_platform_state.atom__NET_WM_WINDOW_TYPE_NORMAL,
                    1);

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

    XStoreName(linux_platform_state.display, linux_platform_state.window_handle, "trader");

    XSizeHints *size_hints           = XAllocSizeHints();
    XWMHints   *window_manager_hints = XAllocWMHints();
    XClassHint *class_hints          = XAllocClassHint();

    size_hints->flags       = PMinSize | PMaxSize | PWinGravity;

    size_hints->min_width   = 50;
    size_hints->min_height  = 50;

    size_hints->max_width   = size_hints->max_height = (1UL << 16UL);

    size_hints->win_gravity = NorthWestGravity;

    window_manager_hints->flags         |= InputHint | StateHint;

    window_manager_hints->input          = True;
    window_manager_hints->initial_state  = NormalState;

    class_hints->res_name  = (char *) "trader";
    class_hints->res_class = (char *) "trader";

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

    // NOTE(antonio): (inso) make the window visible
    XMapWindow(linux_platform_state.display, linux_platform_state.window_handle);

    {
      typedef GLXContext (glXCreateContextAttribsARB_Function)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
      typedef void       (glXSwapIntervalEXT_Function)        (Display *dpy, GLXDrawable drawable, int interval);
      typedef int        (glXSwapIntervalMESA_Function)       (unsigned int interval);
      typedef int        (glXGetSwapIntervalMESA_Function)    (void);
      typedef int        (glXSwapIntervalSGI_Function)        (int interval);

      const char *glx_exts =
        glXQueryExtensionsString(linux_platform_state.display,
                                 DefaultScreen(linux_platform_state.display));

      glXCreateContextAttribsARB_Function *glXCreateContextAttribsARB = NULL;
      glXSwapIntervalEXT_Function         *glXSwapIntervalEXT = NULL;
      glXSwapIntervalMESA_Function        *glXSwapIntervalMESA = NULL;
      glXGetSwapIntervalMESA_Function     *glXGetSwapIntervalMESA = NULL;
      glXSwapIntervalSGI_Function         *glXSwapIntervalSGI = NULL;


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
                    window_manager_protocols, array_count(window_manager_protocols));

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

#pragma pack(push, 1)
  struct Draw_Data
  {
    V3_f32   vertex;
    RGBA_f32 color;
    V2_f32   uv;
  };
#pragma pack(pop)

  Draw_Data triangle_vertices[] = 
  {
    {V3( 0.5f, -0.5f, 0.0f), rgba(1.0f, 0.0f, 0.0f, 1.0f), V2(1.0f, 0.0f)}, // bottom right
    {V3(-0.5f, -0.5f, 0.0f), rgba(0.0f, 1.0f, 0.0f, 1.0f), V2(0.0f, 0.0f)}, // bottom left
    {V3( 0.5f,  0.5f, 0.0f), rgba(0.0f, 0.0f, 1.0f, 1.0f), V2(1.0f, 1.0f)}, // top right

    {V3(-0.5f, -0.5f, 0.0f), rgba(0.0f, 1.0f, 0.0f, 1.0f), V2(0.0f, 0.0f)}, // bottom left
    {V3(-0.5f,  0.5f, 0.0f), rgba(0.0f, 0.0f, 1.0f, 1.0f), V2(0.0f, 1.0f)}, // top left
    {V3( 0.5f,  0.5f, 0.0f), rgba(1.0f, 0.0f, 0.0f, 1.0f), V2(1.0f, 1.0f)}, // top right
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

  u32 vertex_buffer_index = 0;
  {
    glVertexAttribPointer(vertex_buffer_index,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Draw_Data),
                          (void *) member_offset(Draw_Data, vertex));

    glEnableVertexAttribArray(vertex_buffer_index);  

    vertex_buffer_index++;
    glVertexAttribPointer(vertex_buffer_index,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Draw_Data),
                          (void *) member_offset(Draw_Data, color));

    glEnableVertexAttribArray(vertex_buffer_index);  

    vertex_buffer_index++;
    glVertexAttribPointer(vertex_buffer_index,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Draw_Data),
                          (void *) member_offset(Draw_Data, uv));

    glEnableVertexAttribArray(vertex_buffer_index);  
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
        glGetShaderInfoLog(shader_program, sizeof(info_log), NULL, (GLchar *) info_log);
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

  f64 last_frame_time = platform_get_time_in_seconds();
  b32 first_step      = true;

  glViewport(0, 0, (i32) rect_get_width(&default_client_rect), (i32) rect_get_height(&default_client_rect));

  f32 acc_time =  1.0f;
  f32 dir      = -1.0f;

  i32 vertex_color_location;
  i32 texture_sampler_location;
  i32 transform_location;

  {
    vertex_color_location    = glGetUniformLocation(shader_program, "uniform_scale");
    texture_sampler_location = glGetUniformLocation(shader_program, "texture_sampler");
    transform_location       = glGetUniformLocation(shader_program, "uniform_transform");
  }

  glUniform1i(texture_sampler_location, 0);

  Matrix_f32_4x4 transform = matrix4x4_from_rows(V4(1.0f,  0.0f, 0.0f, 0.0f),
                                                 V4(0.0f, -1.0f, 0.0f, 0.0f),
                                                 V4(0.0f,  0.0f, 1.0f, 0.0f),
                                                 V4(0.0f,  0.0f, 0.0f, 1.0f));
  glUniformMatrix4fv(transform_location, 1, GL_TRUE, transform.values);

  glEnable(GL_DEPTH_TEST);  
  glDepthFunc(GL_LESS);

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ZERO);

  global_running = true;
  while (global_running)
  {
    if (XEventsQueued(linux_platform_state.display, QueuedAlready))
    {
      x11_handle_events();
    }

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

    {
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUniform1f(vertex_color_location, acc_time);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_atlas_texture);

      glBindVertexArray(vertex_buffer_reader);
      glDrawArrays(GL_TRIANGLES, 0, array_count(triangle_vertices));
    }

    glXSwapBuffers(linux_platform_state.display, linux_platform_state.window_handle);
    last_frame_time = platform_get_time_in_seconds();

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

  return(0);
}
