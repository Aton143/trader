#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#define UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NO_VA_START_VALIDATION
#define OEMRESOURCE

#include <windows.h>
#include <windowsx.h>

#include <memoryapi.h>
#include <sysinfoapi.h>
#include <fileapi.h>
#include <intrin.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <commdlg.h>

#define TLS_MAX_PACKET_SIZE (16384 + 512)

#include <d3d11_3.h>
#include <d3dcompiler.h>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "comdlg32.lib")

#define safe_release(releasable) if (releasable) releasable->Release()

#include "..\trader.h"

global b32 global_running        = false;
global b32 global_window_resized = false;

internal b32 is_vk_down(i32 vk)
{
  b32 vk_down = (GetKeyState(vk) & 0x8000) != 0;
  return(vk_down);
}

internal u32 win32_network_thread(void *arg)
{
  Thread_Context *net_thread_context = (Thread_Context *) arg;
  expect(net_thread_context != NULL);

  Network_State network_state = {};
  network_startup(&network_state);

  String_Const_utf8 host_name = string_literal_init_type("echo.websocket.events", utf8);
  // String_Const_utf8 host_name = string_literal_init_type("finnhub.io", utf8);

  String_Const_utf8 query_path = string_literal_init_type("", utf8);

  u16 port = 443;

  Socket tls_socket;
  Network_Return_Code net_result = network_connect(&network_state, &tls_socket, host_name, port);
  expect(net_result == network_ok);

  Buffer request_header = stack_alloc_buffer(1024);
  zero_buffer(&request_header);

  request_header.used =
    (u64) stbsp_snprintf((char *) request_header.data,
                         (int)    request_header.size,
                         "GET / HTTP/1.1\r\n"
                         //" GET /api/v1/search?q=apple HTTP/1.1\r\n"
                         // "Host: %s:443\r\n"
                         "Host: %s\r\n"
                         // "Accept: text/*\r\n"
                         "Accept-Encoding: identity\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
                         "Sec-WebSocket-Version: 13\r\n"
                         "\r\n",
                         (char *) host_name.str);

  net_result = network_send_simple(&network_state, &tls_socket, &request_header);
  expect(net_result == network_ok);

  Buffer receive_buffer = stack_alloc_buffer(2048);
  zero_buffer(&receive_buffer);

  net_result = network_receive_simple(&network_state, &tls_socket, &receive_buffer);
  expect(net_result == network_ok);

  i32 disable_nagle = 1;
  i32 disable_nagle_res = setsockopt(tls_socket.socket,
                                     IPPROTO_TCP,
                                     TCP_NODELAY,
                                     (char *) &disable_nagle,
                                     sizeof(disable_nagle));
  expect(disable_nagle_res == 0);

  Buffer to_send = buffer_from_string_literal_type("Vete a la chingada!");
  net_result = network_websocket_send_simple(net_thread_context,
                                             &network_state,
                                             &tls_socket,
                                             &to_send,
                                             websocket_opcode_text);
  expect(net_result == network_ok);

  WebSocket_Frame_Header header;
  zero_buffer(&receive_buffer);
  net_result = network_websocket_receive_simple(net_thread_context,
                                                &network_state,
                                                &tls_socket,
                                                &receive_buffer,
                                                &header);

  /*
  switch (header.opcode)
  {
    case websocket_opcode_continuation:
    {

    } break;
    case websocket_opcode_text:
    {

    } break;
    case websocket_opcode_binary:
    {

    } break;
    case websocket_opcode_close:
    {

    } break;
    case websocket_opcode_ping:
    {
      net_result = network_websocket_send_simple(&network_state, &tls_socket, &receive_buffer, websocket_opcode_pong);
      expect(net_result == network_ok);
    } break;
    case websocket_opcode_pong:
    {
      net_result = network_websocket_receive_simple(&network_state, &tls_socket, &receive_buffer, &header);
      expect(net_result == network_ok);
    } break;
    default:
    {
      meta_log_char("WebSocket: unknown opcode: %d\n", header.opcode);
    } break;
  }
  */

  return(0);
}

internal void win32_message_fiber(void *args)
{
  unused(args);

  while(1)
  {
    MSG message;
    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
    {
      if (message.message == WM_QUIT)
      {
        global_running = false;
      }

      TranslateMessage(&message);
      DispatchMessageW(&message);
    }

    SwitchToFiber(win32_global_state.main_fiber_address);
  }
}

internal LRESULT win32_window_procedure(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;

  UI_Context *ui = ui_get_context();

  switch (message)
  {
    case WM_GETMINMAXINFO:
    {
      MINMAXINFO *window_tracking_info = (MINMAXINFO *) lparam;
      window_tracking_info->ptMinTrackSize = {400, 300};
    } break;

    // TODO(antonio): when y ~= 0, mouse is registered as not in client
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE: // NOTE(antonio): NC - non-client
    {
      POINT mouse_pos =
      {
        (LONG) GET_X_LPARAM(lparam),
        (LONG) GET_Y_LPARAM(lparam)
      };

      if (win32_global_state.nonclient_mouse_button != 0)
      {
        if ((GET_X_LPARAM(win32_global_state.nonclient_mouse_pos) != mouse_pos.x) ||
            (GET_Y_LPARAM(win32_global_state.nonclient_mouse_pos) != mouse_pos.y))
        {
          DefWindowProcW(window_handle,
                         win32_global_state.nonclient_mouse_button,
                         HTCAPTION,
                         win32_global_state.nonclient_mouse_pos);
          win32_global_state.nonclient_mouse_button = 0;
        }
      }

      Mouse_Area cur_mouse_area = (message == WM_MOUSEMOVE) ? mouse_area_in_client : mouse_area_other;
      if (ui->mouse_area != cur_mouse_area)
      {
        b32 tracking_result = false;
        if (ui->mouse_area != mouse_area_out_client)
        {
          TRACKMOUSEEVENT cancel_previous_tracking = {sizeof(cancel_previous_tracking), TME_CANCEL, window_handle, 0};

          tracking_result = TrackMouseEvent(&cancel_previous_tracking);
          expect_message(tracking_result, "expected to cancel previous tracking");
        }

        DWORD new_tracking_flags = (cur_mouse_area == mouse_area_other) ? (TME_LEAVE | TME_NONCLIENT) : (TME_LEAVE);
        TRACKMOUSEEVENT new_tracking = {sizeof(new_tracking), new_tracking_flags, window_handle, 0};

        tracking_result = TrackMouseEvent(&new_tracking);
        expect_message(tracking_result, "expected to begin new tracking");

        ui->mouse_area = cur_mouse_area;
      }

      // WM_NCMOUSEMOVE are provided in absolute coordinates.
      if ((message == WM_NCMOUSEMOVE) && (ScreenToClient(window_handle, &mouse_pos) == FALSE))
      {
        break;
      }

      ui->mouse_delta = 
      {
        (f32) mouse_pos.x - ui->mouse_pos.x,
        (f32) mouse_pos.y - ui->mouse_pos.y,
      };

      printf("Mouse delta: %f %f\n", ui->mouse_delta.x, ui->mouse_delta.y);

      ui->mouse_pos = 
      {
        (f32) mouse_pos.x,
        (f32) mouse_pos.y
      };
    } break;

    case WM_NCLBUTTONDOWN:
    {
      if (wparam == HTCAPTION)
      {
        win32_global_state.nonclient_mouse_pos    = lparam;
        win32_global_state.nonclient_mouse_button = message;
      }
      else
      {
        result = DefWindowProc(window_handle, message, wparam, lparam);
      }
    } break;

    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
    {
      ui->mouse_pos = {max_f32, max_f32};
      ui->mouse_area = mouse_area_out_client;
    } break;

    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
      Mouse_Event mouse_event = mouse_event_none;
      if ((message == WM_LBUTTONDOWN) || (message == WM_LBUTTONDBLCLK)) {mouse_event = mouse_event_lclick;}
      if ((message == WM_RBUTTONDOWN) || (message == WM_RBUTTONDBLCLK)) {mouse_event = mouse_event_rclick;}
      if ((message == WM_MBUTTONDOWN) || (message == WM_MBUTTONDBLCLK)) {mouse_event = mouse_event_mclick;}

      ui->cur_frame_mouse_event |= mouse_event;

      if ((ui->cur_frame_mouse_event == mouse_event_none) && (GetCapture == NULL))
      {
        SetCapture(window_handle);
      }
    } break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
      Mouse_Event mouse_event_to_remove = mouse_event_none;
      if (message == WM_LBUTTONUP) {mouse_event_to_remove = mouse_event_lclick;}
      if (message == WM_RBUTTONUP) {mouse_event_to_remove = mouse_event_rclick;}
      if (message == WM_MBUTTONUP) {mouse_event_to_remove = mouse_event_mclick;}

      ui->cur_frame_mouse_event &= ~mouse_event_to_remove;

      if ((ui->cur_frame_mouse_event == mouse_event_none) && (GetCapture() == window_handle))
      {
        ReleaseCapture();
      }
    }

    case WM_MOUSEWHEEL:
    {
      ui->mouse_wheel_delta = {0.0f, (f32) GET_WHEEL_DELTA_WPARAM(wparam) / (f32) WHEEL_DELTA};
    } break;
    case WM_MOUSEHWHEEL:
    {
      ui->mouse_wheel_delta = {(f32) -GET_WHEEL_DELTA_WPARAM(wparam) / (f32) WHEEL_DELTA, 0.0f};
    } break;

    case WM_SETFOCUS:  // NOTE(antonio): after the window has gained focus
    case WM_KILLFOCUS: // NOTE(antonio): right *before* the window loses focus
    {
      win32_global_state.focus_event = (message == WM_SETFOCUS) ? focus_event_gain : focus_event_lose;
    } break;

    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    {
      b32 is_key_down = ((message == WM_KEYDOWN) || (message == WM_SYSKEYDOWN));
      if (wparam < 256)
      {
        ui_add_key_event(key_mod_event_control, is_vk_down(VK_CONTROL));
        ui_add_key_event(key_mod_event_shift,   is_vk_down(VK_SHIFT));
        ui_add_key_event(key_mod_event_alt,     is_vk_down(VK_MENU));
        ui_add_key_event(key_mod_event_super,   is_vk_down(VK_APPS));

        u64 vk = (u64) wparam;
        if ((wparam == VK_RETURN) && (HIWORD(lparam) & KF_EXTENDED))
        {
          vk = TRADER_KEYPAD_ENTER;
        }

        // Submit key event
        Key_Event key = platform_convert_key_to_our_key(vk);
        if (key != key_event_none)
        {
          ui_add_key_event(key, is_key_down);
        }

        /*
        // Submit individual left/right modifier events
        if (vk == VK_SHIFT)
        {
          // Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
          if (IsVkDown(VK_LSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode); }
          if (IsVkDown(VK_RSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode); }
        }
        else if (vk == VK_CONTROL)
        {
          if (IsVkDown(VK_LCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode); }
          if (IsVkDown(VK_RCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode); }
        }
        else if (vk == VK_MENU)
        {
          if (IsVkDown(VK_LMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode); }
          if (IsVkDown(VK_RMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode); }
        }
        */
      }

      if (wparam == VK_ESCAPE)
      {
        global_running = false;
      }
      result = DefWindowProc(window_handle, message, wparam, lparam);
    } break;

    case WM_SIZE:
    {
      global_window_resized = true;
      //result = DefWindowProc(window_handle, message, wparam, lparam);
    } break;

    case WM_SIZING:
    {
      result = TRUE;
    } break;

    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
    {
      SetTimer(window_handle, 1, 2, NULL);
    } break;

    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
    {
      KillTimer(window_handle, 1);
      result = DefWindowProcW(window_handle, message, wparam, lparam);
    } break;

    case WM_TIMER:
    {
      if (wparam == 1)
      {
        SwitchToFiber(win32_global_state.main_fiber_address);
        result = DefWindowProcW(window_handle, message, wparam, lparam);
      }
    } break;

    case WM_SYSCOMMAND:
    {
      switch (wparam & 0xff0)
      {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
        {

        } break;

        case SC_KEYMENU:
        {

        } break;
      }

      result = DefWindowProcW(window_handle, message, wparam, lparam);
    } break;

    case WM_QUIT:
    case WM_DESTROY:
    case WM_CLOSE:
    {
      global_running = false;
    } break;

    default:
    {
      result = DefWindowProcW(window_handle, message, wparam, lparam);
    } break;
  }

  return(result);
}

int CALL_CONVENTION
WinMain(HINSTANCE instance,
        HINSTANCE previous_instance,
        LPSTR     command_line,
        int       show_command)
{
  unused(instance);
  unused(previous_instance);
  unused(command_line);
  unused(show_command);

  platform_common_init();

  Common_Render_Context *common_render = render_get_common_context();

#if !SHIP_MODE
  ID3D11Debug *debug = NULL;
#endif

  utf16 _exe_file_path[MAX_PATH] = {};
  GetModuleFileNameW(NULL, _exe_file_path, array_count(_exe_file_path));

  String_Const_utf16 exe_file_path = string_const_utf16((utf16 *) _exe_file_path);
  unused(exe_file_path);

  SetCurrentDirectoryW((LPCWSTR) exe_file_path.str);

  HANDLE iocp_handle = INVALID_HANDLE_VALUE;
  {
    // TODO(antonio): use completion key to distinguish handles?
    iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (iocp_handle == INVALID_HANDLE_VALUE)
    {
      expect_message(false, "could not create an I/O completion port");
    }

    win32_global_state.notify_iocp = iocp_handle;
  }

  String_Const_utf8 notify_dir = string_literal_init_type("..\\src\\platform_win32\\", utf8);

  platform_push_notify_dir(notify_dir.str, notify_dir.size);
  platform_start_collect_notifications();

  File_Buffer arial_font = platform_open_and_read_entire_file(&win32_global_state.global_arena,
                                                              default_font_path.str,
                                                              default_font_path.size);

  // NOTE(antonio): default font on Windows is Arial
  common_render->default_font =
    platform_open_and_read_entire_file(&win32_global_state.global_arena,
                                       default_font_path.str,
                                       default_font_path.size);

  render_atlas_initialize(&win32_global_state.global_arena,
                          win32_global_state.render_context.atlas,
                          &arial_font,
                          (f32 *) default_font_heights,
                          array_count(default_font_heights),
                          512, 512);

  Texture_Atlas *atlas = win32_global_state.render_context.atlas;

  {
    WNDCLASSEXW window_class = {};

    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = win32_window_procedure;
    window_class.hInstance     = instance;
    window_class.hIcon         = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    window_class.lpszClassName = L"trader";
    window_class.hIconSm       = LoadIconW(0, (LPCWSTR) IDI_APPLICATION);

    ATOM register_class_result = RegisterClassExW(&window_class);

    if (!register_class_result)
    {
      MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
      return GetLastError();
    }

    win32_global_state.window_handle =
      CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
                      window_class.lpszClassName,
                      L"trader",
                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT,
                      NULL, NULL,
                      instance, NULL);

    if (!win32_global_state.window_handle)
    {
      MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
      return GetLastError();
    }
  }

  // Thread_Handle network_thread = platform_create_thread(&win32_network_thread, &thread_contexts[1]);
  // unused(network_thread);

  if (ShowWindow(win32_global_state.window_handle, SW_NORMAL) && UpdateWindow(win32_global_state.window_handle))
  {
    // NOTE(antonio): initializing Direct3D 11
    ID3D11Device1 *device1 = NULL;
    ID3D11Device2 *device2 = NULL;
    ID3D11Device3 *device  = NULL;

    ID3D11DeviceContext1 *device_context = NULL;
    {
      ID3D11Device *base_device = NULL;
      ID3D11DeviceContext *base_device_context = NULL;

      D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};
      UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if !SHIP_MODE
      creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

      HRESULT result =
        D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 
                          0, creation_flags, 
                          feature_levels, array_count(feature_levels), 
                          D3D11_SDK_VERSION, &base_device, 
                          0, &base_device_context);

      if (FAILED(result))
      {
        MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
        return GetLastError();
      }

      result = base_device->QueryInterface(__uuidof(ID3D11Device1), (void **) &device1);
      expect(SUCCEEDED(result));
      base_device->Release();

      result = device1->QueryInterface(__uuidof(ID3D11Device2), (void **) &device2);
      expect(SUCCEEDED(result));
      device1->Release();

      result = device2->QueryInterface(__uuidof(ID3D11Device3), (void **) &device);
      expect(SUCCEEDED(result));
      device2->Release();

      result = base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **) &device_context);
      expect(SUCCEEDED(result));
      base_device_context->Release();
    }

#if !SHIP_MODE
    device->QueryInterface(__uuidof(ID3D11Debug), (void **) &debug);
    if (debug)
    {
      ID3D11InfoQueue *info_queue = NULL;
      if (SUCCEEDED(debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void **) &info_queue)))
      {
        info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
        info_queue->Release();
      }
      debug->Release();
    }
#endif

    IDXGISwapChain1 *swap_chain = NULL;
    { 
      IDXGIFactory2 *dxgi_factory = NULL;
      {
        IDXGIDevice1 *dxgi_device = NULL;

        HRESULT result = device->QueryInterface(__uuidof(IDXGIDevice1), (void **) &dxgi_device);
        expect(SUCCEEDED(result));

        IDXGIAdapter *dxgi_adapter = NULL;
        result = dxgi_device->GetAdapter(&dxgi_adapter);
        expect(SUCCEEDED(result));
        dxgi_device->Release();

        DXGI_ADAPTER_DESC adapter_description = {};
        dxgi_adapter->GetDesc(&adapter_description);

        OutputDebugStringW(L"Graphics Device: ");
        OutputDebugStringW(adapter_description.Description);
        OutputDebugStringW(L"\n");

        result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void **) &dxgi_factory);
        expect(SUCCEEDED(result));
        dxgi_adapter->Release();
      }

      DXGI_SWAP_CHAIN_DESC1 swap_chain_description = {};

      swap_chain_description.Width  = 0;  // use window width
      swap_chain_description.Height = 0;  // use window height
      swap_chain_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

      swap_chain_description.SampleDesc.Count   = 1;
      swap_chain_description.SampleDesc.Quality = 0;

      swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swap_chain_description.BufferCount = 2;

      swap_chain_description.Scaling     = DXGI_SCALING_STRETCH;
      swap_chain_description.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swap_chain_description.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
      swap_chain_description.Flags       = 0;

      HRESULT result = dxgi_factory->CreateSwapChainForHwnd(device, win32_global_state.window_handle,
                                                            &swap_chain_description,
                                                            0, 0, &swap_chain);
      expect(SUCCEEDED(result));

      dxgi_factory->Release();
    }

    ID3D11RenderTargetView *frame_buffer_view = NULL;
    {
      ID3D11Texture2D *frame_buffer = NULL;
      HRESULT result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &frame_buffer);
      expect(SUCCEEDED(result));

      result = device->CreateRenderTargetView(frame_buffer, 0, &frame_buffer_view);
      expect(SUCCEEDED(result));
      frame_buffer->Release();
    }

    {
      win32_global_state.render_context.swap_chain           = swap_chain;
      win32_global_state.render_context.device               = device;
      win32_global_state.render_context.device_context       = device_context;
    };

    String_Const_utf8 shader_source_path   = string_literal_init_type("..\\src\\platform_win32\\shaders.hlsl", utf8);
    Handle           *shader_source_handle = make_handle(shader_source_path, Handle_Kind_File);

    Vertex_Shader renderer_vertex_shader = {};
    Pixel_Shader  renderer_pixel_shader = {};

    ID3DBlob *vertex_shader_blob =
      (ID3DBlob *) render_load_vertex_shader(shader_source_handle, &renderer_vertex_shader, true);
    render_load_pixel_shader(shader_source_handle, &renderer_pixel_shader, true);

    ID3D11InputLayout *input_layout = NULL;
    {
      D3D11_INPUT_ELEMENT_DESC input_element_description[] =
      {
#define INSTANCE_BUFFER_SLOT 0
        // NOTE(antonio): instance buffer data
        {
          "INSTANCE_SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          0, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_SIZE", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_EDGE_SOFTNESS", 0, DXGI_FORMAT_R32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_BORDER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_UV", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        }, {
          "INSTANCE_UV", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        },
      };

      HRESULT result = device->CreateInputLayout(input_element_description, array_count(input_element_description),
                                                 vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize(),
                                                 &input_layout);
      expect(SUCCEEDED(result));
      vertex_shader_blob->Release();
    }

    ID3D11Buffer *instance_buffer = NULL;
    {
      D3D11_BUFFER_DESC instance_buffer_description = {};

      instance_buffer_description.ByteWidth      = (u32) common_render->render_data.size;
      instance_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
      instance_buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      instance_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      HRESULT result = device->CreateBuffer(&instance_buffer_description, NULL, &instance_buffer);
      expect(SUCCEEDED(result));
    }

    String_Const_utf8 triangle_shader_source_path =
      string_literal_init_type("..\\src\\platform_win32\\triangle_shaders.hlsl", utf8);
    Handle *triangle_shader_source_handle = make_handle(triangle_shader_source_path, Handle_Kind_File);

    Vertex_Shader triangle_vertex_shader = {};
    Pixel_Shader  triangle_pixel_shader = {};

    ID3DBlob *triangle_vertex_shader_blob =
      (ID3DBlob *) render_load_vertex_shader(triangle_shader_source_handle, &triangle_vertex_shader, true);
    render_load_pixel_shader(triangle_shader_source_handle, &triangle_pixel_shader, true);

    ID3D11InputLayout *triangle_input_layout = NULL;
    {
      D3D11_INPUT_ELEMENT_DESC input_element_description[] =
      {
#define INSTANCE_BUFFER_SLOT 0
        {
          "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
        }, {
          "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
        }, {
          "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
        },
      };

      HRESULT result = device->CreateInputLayout(input_element_description,
                                                 array_count(input_element_description),
                                                 triangle_vertex_shader_blob->GetBufferPointer(),
                                                 triangle_vertex_shader_blob->GetBufferSize(),
                                                 &triangle_input_layout);
      expect(SUCCEEDED(result));
      triangle_vertex_shader_blob->Release();
    }

    ID3D11Buffer *vertex_buffer = NULL;
    {
      D3D11_BUFFER_DESC vertex_buffer_description = {};

      vertex_buffer_description.ByteWidth      = (u32) common_render->triangle_render_data.size;
      vertex_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
      vertex_buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      vertex_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      HRESULT result = device->CreateBuffer(&vertex_buffer_description, NULL, &vertex_buffer);
      expect(SUCCEEDED(result));
    }

    ID3D11SamplerState *sampler_state = NULL;
    {
      D3D11_SAMPLER_DESC sampler_description = {};

      sampler_description.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      sampler_description.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
      sampler_description.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
      sampler_description.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
      sampler_description.BorderColor[0] = 1.0f;
      sampler_description.BorderColor[1] = 1.0f;
      sampler_description.BorderColor[2] = 1.0f;
      sampler_description.BorderColor[3] = 1.0f;
      sampler_description.ComparisonFunc = D3D11_COMPARISON_NEVER;
      sampler_description.MinLOD         = 0;
      sampler_description.MaxLOD         = D3D11_FLOAT32_MAX;

      HRESULT result = device->CreateSamplerState(&sampler_description, &sampler_state);
      expect(SUCCEEDED(result));
    }

    ID3D11BlendState *transparent_blend_state = NULL;
    {
      D3D11_BLEND_DESC blend_description = {};

      blend_description.AlphaToCoverageEnable                 = FALSE;
      blend_description.IndependentBlendEnable                = FALSE;

      blend_description.RenderTarget[0].BlendEnable           = TRUE;

      blend_description.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
      blend_description.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      blend_description.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;

      blend_description.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
      blend_description.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
      blend_description.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;

      blend_description.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      HRESULT result = device->CreateBlendState(&blend_description, &transparent_blend_state);
      expect(SUCCEEDED(result));
    }

    ID3D11RasterizerState* rasterizer_state = {};
    {
      D3D11_RASTERIZER_DESC rasterizer_description = {};

      rasterizer_description.FillMode              = D3D11_FILL_SOLID;
      rasterizer_description.CullMode              = D3D11_CULL_NONE;
      rasterizer_description.FrontCounterClockwise = TRUE;
      rasterizer_description.ScissorEnable         = TRUE;

      HRESULT result = device->CreateRasterizerState(&rasterizer_description, &rasterizer_state);
      expect(SUCCEEDED(result));
    }

    ID3D11ShaderResourceView *font_texture_view = NULL;
    {
      D3D11_TEXTURE2D_DESC texture_description = {};
      {
        texture_description.Width            = (u32) atlas->bitmap.height;
        texture_description.Height           = (u32) atlas->bitmap.width;
        texture_description.MipLevels        = 1;
        texture_description.ArraySize        = 1;
        texture_description.Format           = DXGI_FORMAT_R8_UNORM;
        texture_description.SampleDesc.Count = 1;
        texture_description.Usage            = D3D11_USAGE_DEFAULT;
        texture_description.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
        texture_description.CPUAccessFlags   = 0;
      }

      D3D11_SUBRESOURCE_DATA subresource = {};
      {
        subresource.pSysMem          = atlas->bitmap.alpha;
        subresource.SysMemPitch      = texture_description.Width * 1;
        subresource.SysMemSlicePitch = 0;
      }

      ID3D11Texture2D *texture_2d = NULL;
      HRESULT result = device->CreateTexture2D(&texture_description, &subresource, &texture_2d);

      expect(SUCCEEDED(result));

      D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};
      {
        shader_resource_view_description.Format                    = DXGI_FORMAT_R8_UNORM;
        shader_resource_view_description.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_description.Texture2D.MipLevels       = texture_description.MipLevels;
        shader_resource_view_description.Texture2D.MostDetailedMip = 0;
      }

      result = device->CreateShaderResourceView(texture_2d, &shader_resource_view_description, &font_texture_view);
      texture_2d->Release();

      expect(SUCCEEDED(result));
    }

    ID3D11Buffer *constant_buffer = NULL;
    {
      D3D11_BUFFER_DESC constant_buffer_description = {};

      // NOTE(antonio): ByteWidth must be a multiple of 16, per the docs
      constant_buffer_description.ByteWidth      = (sizeof(Constant_Buffer) + 0xf) & 0xfffffff0;
      constant_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
      constant_buffer_description.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      HRESULT result = device->CreateBuffer(&constant_buffer_description, NULL, &constant_buffer);
      expect(SUCCEEDED(result));
    }

    ID3D11DepthStencilState *depth_stencil_state = NULL;
    {
      D3D11_DEPTH_STENCIL_DESC depth_stencil_state_description = {};

      depth_stencil_state_description.DepthEnable                  = TRUE;
      depth_stencil_state_description.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
      depth_stencil_state_description.DepthFunc                    = D3D11_COMPARISON_LESS_EQUAL;
      depth_stencil_state_description.StencilEnable                = TRUE;
      depth_stencil_state_description.StencilReadMask              = 0xff;
      depth_stencil_state_description.StencilWriteMask             = 0xff;
      depth_stencil_state_description.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
      depth_stencil_state_description.BackFace                     = depth_stencil_state_description.FrontFace;

      HRESULT result = device->CreateDepthStencilState(&depth_stencil_state_description, &depth_stencil_state);
      expect(SUCCEEDED(result));
    }

    ID3D11Texture2D        *depth_stencil_texture = NULL;
    ID3D11DepthStencilView *depth_stencil_view = NULL;

#if !SHIP_MODE
    b32              save_current_frame_buffer = false;
    ID3D11Texture2D *copy_frame_buffer_texture = NULL;

    unused(save_current_frame_buffer);
#endif

    UI_Context *ui = &win32_global_state.ui_context;

    global_running = true;
    global_window_resized = true;

    u64 last_hpt = platform_get_high_precision_timer();
    u64 last_pts = platform_get_processor_time_stamp();

    f64 last_frame_time           = platform_convert_high_precision_time_to_seconds(last_hpt);
    u64 last_frame_time_in_cycles = 0;

    win32_global_state.dt = 0.0;

    String_Const_utf8 text_to_render = string_literal_init_type("abcdefg", utf8);
      /*string_literal_init_type("abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ "
                                 "1234567890"
                                 "!@#$%^&*()"
                                 "{}|[]\\;':\",./<>?-=_+`~", utf8);*/

    File_Buffer file = {};
    String_Const_utf8 file_str = {};

#pragma warning(disable:4302)
    cursors[cursor_kind_none]._handle = NULL;

    cursors[cursor_kind_pointer]._handle =
      (HCURSOR) LoadImage(NULL, MAKEINTRESOURCEW(IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    cursors[cursor_kind_finger_pointer]._handle =
      (HCURSOR) LoadImage(NULL, MAKEINTRESOURCEW(IDC_HAND), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    cursors[cursor_kind_left_right_direction]._handle =
      (HCURSOR) LoadImage(NULL, MAKEINTRESOURCEW(IDC_SIZEWE), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    cursors[cursor_kind_up_down_direction]._handle =
      (HCURSOR) LoadImage(NULL, MAKEINTRESOURCEW(IDC_SIZENS), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    cursors[cursor_kind_text_selection]._handle =
      (HCURSOR) LoadImage(NULL, MAKEINTRESOURCEW(IDC_IBEAM), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
#pragma warning(default:4302)

    f32 panel_floats[16]  = {0.25f};
    u32 panel_float_index = 0;

    f32 cylinder_top_radius = 0.50f;
    f32 cylinder_sector_count = 3.0f;

    // NOTE(antonio): experimental change
    win32_global_state.main_fiber_address = ConvertThreadToFiber(NULL);
    expect_message(win32_global_state.main_fiber_address != NULL, "expected to use fiber path");

    void *win32_message_fiber_handle = CreateFiber(0, &win32_message_fiber, NULL);
    expect_message(win32_global_state.main_fiber_address != NULL, "could not create a fiber for messages");

    Vertex_Buffer_Element cube_vertices[] = 
    {
#include "../trader_cube_vertices.h"
    };

    f32 acc_time = 9.0f;

    Matrix_f32_4x4 view       = matrix4x4_diagonals(1.0f, 1.0f, 1.0f, 1.0f);
    Matrix_f32_4x4 projection = matrix4x4_symmetric_projection(1.0f, 100.0f, 1.0f, 1.0f);

    V3_f32 points[] = {V3(0.0f, 0.0f, 0.0f), V3(0.0f, 0.0f, 1.0f)};

    while (global_running)
    {
      TIMED_BLOCK_START();

      SwitchToFiber(win32_message_fiber_handle);

      if (global_window_resized && !IsIconic(win32_global_state.window_handle))
      {
        global_window_resized = false;

        {
          Rect_f32 new_client_rect = {};

          RECT win32_client_rect = {};
          GetClientRect(platform_get_global_state()->window_handle, &win32_client_rect);

          new_client_rect.x0 = 0;
          new_client_rect.y0 = 0;

          new_client_rect.x1 = (f32) (win32_client_rect.right  - win32_client_rect.left);
          new_client_rect.y1 = (f32) (win32_client_rect.bottom - win32_client_rect.top);

          render_set_client_rect(new_client_rect);
        }

        device_context->OMSetRenderTargets(0, 0, 0);
        frame_buffer_view->Release();

        HRESULT result = swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        // expect(SUCCEEDED(result));

        ID3D11Texture2D* frame_buffer = NULL;
        result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &frame_buffer);
        // expect(SUCCEEDED(result));

        result = device->CreateRenderTargetView(frame_buffer, NULL, &frame_buffer_view);
        // expect(SUCCEEDED(result));

        frame_buffer->Release();

        safe_release(depth_stencil_texture);
        depth_stencil_texture = NULL;
        {
          D3D11_TEXTURE2D_DESC depth_stencil_texture_desc = {};

          Rect_f32 client_rect = render_get_client_rect();

          depth_stencil_texture_desc.Width              = (UINT) client_rect.x1;
          depth_stencil_texture_desc.Height             = (UINT) client_rect.y1;
          depth_stencil_texture_desc.MipLevels          = 1;
          depth_stencil_texture_desc.ArraySize          = 1;
          depth_stencil_texture_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
          depth_stencil_texture_desc.SampleDesc.Count   = 1;
          depth_stencil_texture_desc.SampleDesc.Quality = 0;
          depth_stencil_texture_desc.Usage              = D3D11_USAGE_DEFAULT;
          depth_stencil_texture_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
          depth_stencil_texture_desc.CPUAccessFlags     = 0;
          depth_stencil_texture_desc.MiscFlags          = 0;

          result = device->CreateTexture2D(&depth_stencil_texture_desc, NULL, &depth_stencil_texture);
          // expect(SUCCEEDED(result));
        }

        safe_release(depth_stencil_view);
        depth_stencil_view = NULL;
        {
          D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};

          depth_stencil_view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
          depth_stencil_view_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
          depth_stencil_view_desc.Texture2D.MipSlice = 0;

          result = device->CreateDepthStencilView(depth_stencil_texture,
                                                  &depth_stencil_view_desc,
                                                  &depth_stencil_view);
          // expect(SUCCEEDED(result));
        } 

#if !SHIP_MODE
        safe_release(copy_frame_buffer_texture);
        copy_frame_buffer_texture = NULL;
        {
          D3D11_TEXTURE2D_DESC copy_frame_buffer_texture_desc = {};

          Rect_f32 client_rect = render_get_client_rect();

          copy_frame_buffer_texture_desc.Width              = (UINT) client_rect.x1;
          copy_frame_buffer_texture_desc.Height             = (UINT) client_rect.y1;
          copy_frame_buffer_texture_desc.MipLevels          = 1;
          copy_frame_buffer_texture_desc.ArraySize          = 1;
          copy_frame_buffer_texture_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
          copy_frame_buffer_texture_desc.SampleDesc.Count   = 1;
          copy_frame_buffer_texture_desc.SampleDesc.Quality = 0;
          copy_frame_buffer_texture_desc.Usage              = D3D11_USAGE_DEFAULT;
          copy_frame_buffer_texture_desc.BindFlags          = D3D11_BIND_RENDER_TARGET;
          copy_frame_buffer_texture_desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
          copy_frame_buffer_texture_desc.MiscFlags          = 0;

          result = device->CreateTexture2D(&copy_frame_buffer_texture_desc, NULL, &copy_frame_buffer_texture);
        }
#endif
      }

      Rect_f32 client_rect = render_get_client_rect();
      platform_collect_notifications();

      ui_initialize_frame();
      panel_float_index = 0;

      ui_push_background_color(rgba_from_u8(0, 0, 100, 255));
      ui_make_panel(axis_split_vertical, &panel_floats[panel_float_index++], string_literal_init_type("first", utf8));
      ui_do_string(string_literal_init_type("Hello World!", utf8));
      ui_do_formatted_string("Mouse (%.0f, %.0f)", ui->mouse_pos.x, ui->mouse_pos.y);
      ui_do_slider_f32(string_literal_init_type("Cylinder Top Radius", utf8), &cylinder_top_radius, 0.0f, 1.0f);
      ui_do_slider_f32(string_literal_init_type("Cylinder Sector Count", utf8), &cylinder_sector_count, 3.0f, 100.0f);

      ui_prepare_render_from_panels(ui_get_sentinel_panel(), client_rect);

      u32 initial_draw_count = (u32) (win32_global_state.render_context.render_data.used / sizeof(Instance_Buffer_Element));
      ui_flatten_draw_layers();

    // make_cylinder(&common_render->triangle_render_data, 1.0f, cylinder_top_radius, 1.0f, (i32) (cylinder_sector_count), 1);
      make_cylinder_along_path(&common_render->triangle_render_data,
                               points, array_count(points), 0.5f, 16);

      // NOTE(antonio): instances
      FLOAT background_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
      Constant_Buffer constant_buffer_items = {};
      {
        device_context->ClearRenderTargetView(frame_buffer_view, background_color);
        device_context->OMSetRenderTargets(1, &frame_buffer_view, depth_stencil_view);
        device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
          D3D11_MAPPED_SUBRESOURCE mapped_vertex_buffer = {};
          device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_buffer);

          copy_memory_block(mapped_vertex_buffer.pData,
                            win32_global_state.render_context.triangle_render_data.start,
                            win32_global_state.render_context.triangle_render_data.used);

          device_context->Unmap(vertex_buffer, 0);
        }

        {
          constant_buffer_items.atlas_width   = (f32) atlas->bitmap.width;
          constant_buffer_items.atlas_height  = (f32) atlas->bitmap.height;

          Matrix_f32_4x4 translation = matrix4x4_translate(0.0f, 0.0f, -3.0f);
          Matrix_f32_4x4 x_rotation  = matrix4x4_rotate_about_x(0.0f / 10.0f);
          Matrix_f32_4x4 y_rotation  = matrix4x4_rotate_about_y(0.0f / 10.0f);
          Matrix_f32_4x4 z_rotation  = matrix4x4_rotate_about_z(0.0f / 10.0f);

          constant_buffer_items.model      = matrix4x4_multiply(translation,
                                             matrix4x4_multiply(x_rotation,
                                             matrix4x4_multiply(y_rotation, z_rotation)));

          constant_buffer_items.view       = view;
          constant_buffer_items.projection = projection;

          acc_time += 1.0f / 60.0f;
          if (acc_time > 10.0f) acc_time -= 10.0f;
        }

        {
          D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
          device_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);

          Constant_Buffer *constants_to_fill = (Constant_Buffer *) mapped_subresource.pData;
          copy_struct(constants_to_fill, &constant_buffer_items);

          device_context->Unmap(constant_buffer, 0);
        }

        // NOTE(antonio): triangles
        D3D11_VIEWPORT viewport =
        {
          client_rect.x0, client_rect.y0, 
          rect_get_width(&client_rect), rect_get_height(&client_rect),
          0.0f, 1.0f
        };

        device_context->RSSetViewports(1, &viewport);

        device_context->IASetInputLayout(triangle_input_layout);

        u32 vertex_buffer_strides[] = {sizeof(Vertex_Buffer_Element)};
        u32 vertex_buffer_offsets[] = {0};
        device_context->IASetVertexBuffers(0, 1, &vertex_buffer,
                                           vertex_buffer_strides,
                                           vertex_buffer_offsets);

        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        render_load_vertex_shader(triangle_shader_source_handle, &triangle_vertex_shader);
        device_context->VSSetShader(triangle_vertex_shader.shader, NULL, 0);
        device_context->VSSetConstantBuffers(0, 1, &constant_buffer);

        render_load_pixel_shader(triangle_shader_source_handle, &triangle_pixel_shader);
        device_context->PSSetShader(triangle_pixel_shader.shader, NULL, 0);

        device_context->PSSetShaderResources(0, 1, &font_texture_view);
        device_context->PSSetSamplers(0, 1, &sampler_state);

        device_context->GSSetShader(NULL, NULL, 0);
        device_context->HSSetShader(NULL, NULL, 0);
        device_context->DSSetShader(NULL, NULL, 0);
        device_context->CSSetShader(NULL, NULL, 0);

        f32 blend_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        device_context->OMSetBlendState(transparent_blend_state, blend_factor, 0xffffffff);
        device_context->OMSetDepthStencilState(depth_stencil_state, 0);
        device_context->RSSetState(rasterizer_state);

        D3D11_RECT scissor_rectangle = {(LONG) 0, (LONG) 0, (LONG) client_rect.x1, (LONG) client_rect.y1};
        device_context->RSSetScissorRects(1, &scissor_rectangle);

        u32 triangle_draw_call_count = 
          (u32) (win32_global_state.render_context.triangle_render_data.used / sizeof(Vertex_Buffer_Element));

        expect_message((triangle_draw_call_count % 3) == 0, 
                       "Expected vertex count to be divisible by 3 - "
                       "you realize you're drawing triangles, right?");

        device_context->Draw(triangle_draw_call_count, 0);
      }

      {
        device_context->OMSetRenderTargets(1, &frame_buffer_view, depth_stencil_view);
        device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
          {
            constant_buffer_items.client_width  = client_rect.x1;
            constant_buffer_items.client_height = client_rect.y1;

            constant_buffer_items.atlas_width   = (f32) atlas->bitmap.width;
            constant_buffer_items.atlas_height  = (f32) atlas->bitmap.height;
          }

          D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
          device_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);

          Constant_Buffer *constants_to_fill = (Constant_Buffer *) mapped_subresource.pData;
          copy_struct(constants_to_fill, &constant_buffer_items);

          device_context->Unmap(constant_buffer, 0);
        }

        {
          D3D11_MAPPED_SUBRESOURCE mapped_instance_buffer = {};
          device_context->Map(instance_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_instance_buffer);

          copy_memory_block(mapped_instance_buffer.pData,
                            win32_global_state.render_context.render_data.start,
                            win32_global_state.render_context.render_data.used);

          device_context->Unmap(instance_buffer, 0);
        }

        D3D11_VIEWPORT viewport =
        {
          0.0f, 0.0f,
          client_rect.x1, client_rect.y1,
          0.0f, 1.0f
        };

        device_context->RSSetViewports(1, &viewport);

        device_context->IASetInputLayout(input_layout);

        u32 instance_buffer_strides[] = {sizeof(Instance_Buffer_Element)};
        u32 instance_buffer_offsets[] = {0};
        device_context->IASetVertexBuffers(0, 1, &instance_buffer,
                                           instance_buffer_strides,
                                           instance_buffer_offsets);

        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        render_load_vertex_shader(shader_source_handle, &renderer_vertex_shader);
        device_context->VSSetShader(renderer_vertex_shader.shader, NULL, 0);
        device_context->VSSetConstantBuffers(0, 1, &constant_buffer);

        render_load_pixel_shader(shader_source_handle, &renderer_pixel_shader);
        device_context->PSSetShader(renderer_pixel_shader.shader, NULL, 0);

        device_context->PSSetShaderResources(0, 1, &font_texture_view);
        device_context->PSSetSamplers(0, 1, &sampler_state);

        device_context->GSSetShader(NULL, NULL, 0);
        device_context->HSSetShader(NULL, NULL, 0);
        device_context->DSSetShader(NULL, NULL, 0);
        device_context->CSSetShader(NULL, NULL, 0);

        f32 blend_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        device_context->OMSetBlendState(transparent_blend_state, blend_factor, 0xffffffff);
        device_context->OMSetDepthStencilState(depth_stencil_state, 0);
        device_context->RSSetState(rasterizer_state);

        D3D11_RECT scissor_rectangle = {(LONG) 0, (LONG) 0, (LONG) client_rect.x1, (LONG) client_rect.y1};
        device_context->RSSetScissorRects(1, &scissor_rectangle);

        device_context->DrawInstanced(4, initial_draw_count, 0, 0);

        for (u32 draw_layer_index = 0;
             draw_layer_index < array_count(ui->render_layers);
             ++draw_layer_index)
        {
          u32 instance_count = (u32) (ui->render_layers[draw_layer_index].used / sizeof(Instance_Buffer_Element));
          device_context->DrawInstanced(4,
                                        instance_count,
                                        0,
                                        ui->flattened_draw_layer_indices[draw_layer_index]);
        }
      }

#if 0 && !SHIP_MODE
      if (save_current_frame_buffer)
      {
        save_current_frame_buffer = false;

        ID3D11RenderTargetView* copy_frame_buffer_rtv = NULL;
        {
          D3D11_RENDER_TARGET_VIEW_DESC copy_frame_buffer_rtv_desc = {};

          copy_frame_buffer_rtv_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
          copy_frame_buffer_rtv_desc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
          copy_frame_buffer_rtv_desc.Texture2D.MipSlice = 0;

          device->CreateRenderTargetView(copy_frame_buffer_texture,
                                         &copy_frame_buffer_rtv_desc,
                                         &copy_frame_buffer_rtv);
        }

        device_context->OMSetRenderTargets(1, &copy_frame_buffer_rtv, depth_stencil_view);
        device_context->ClearRenderTargetView(copy_frame_buffer_rtv, background_color);
        device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        device_context->DrawInstanced(4, draw_call_count, 0, 0);

        Arena *temp_arena = get_temp_arena();
        UINT width  = (UINT) render_get_client_rect().x1;
        UINT height = (UINT) render_get_client_rect().y1;

        device_context->Map(copy_frame_buffer_texture, 0, D3D11_MAP_READ, 0, NULL);

        u32 *data = push_array(temp_arena, u32, width * height);
        device->ReadFromSubresource(data, width * 4, width * height * 4, copy_frame_buffer_texture, 0, NULL);

        i32 write_result = stbi_write_png("./debug/framebuffer.png", width, height, 4, data, width * 4);
        expect(write_result);

        device_context->Unmap(copy_frame_buffer_texture, 0);
        safe_release(copy_frame_buffer_rtv);
      }
#endif
      swap_chain->Present(1, 0);

      arena_reset(&win32_global_state.render_context.render_data);
      arena_reset(&win32_global_state.render_context.triangle_render_data);

      meta_collate_timing_records();

      // Post-frame
      win32_global_state.focus_event = focus_event_none;

      ui->mouse_delta            = {0.0f, 0.0f};
      ui->mouse_wheel_delta      = {0.0f, 0.0f};
      ui->prev_frame_mouse_event = ui->cur_frame_mouse_event;

      for (u32 interaction_index = 0;
           interaction_index < array_count(ui->interactions);
           ++interaction_index) 
      {
        b32 had_frames_left = ui->interactions[interaction_index].frames_left > 0;
        unused(had_frames_left);

        UI_Interaction *cur_int = &ui->interactions[interaction_index];
        cur_int->frames_left--;

        if (cur_int->frames_left < 0)
        {
          ui->interactions[interaction_index] = {};
        }
      }

      {
        u64 cur_pts = platform_get_processor_time_stamp();
        u64 cur_hpt = platform_get_high_precision_timer();

        last_frame_time           = platform_convert_high_precision_time_to_seconds(cur_hpt - last_hpt);
        last_frame_time_in_cycles = cur_pts - last_pts;

        last_hpt = cur_hpt;
        last_pts = cur_pts;

        win32_global_state.dt = last_frame_time;
      }

      if (!ui->keep_hot_key) {
        ui->hot_key = nil_key;
      }

      TIMED_BLOCK_END();
    }
  }

#if !SHIP_MODE
  debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
#endif

  return(0);
}

const u32 _timing_records_count = __COUNTER__;
Timing_Record _timing_records[_timing_records_count] = {};

u32 timing_records_count = _timing_records_count;
Timing_Record *timing_records = _timing_records;
