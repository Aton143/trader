#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#define UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>

#include <memoryapi.h>
#include <sysinfoapi.h>
#include <fileapi.h>
#include <intrin.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>

#define TLS_MAX_PACKET_SIZE (16384 + 512)

#include <d3d11_1.h>
#include <d3dcompiler.h>

#pragma comment (lib, "secur32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "..\trader.h"

global_const String_Const_char shader = string_literal_init(
"struct Global_Data\n"
"{\n"
"  float2 resolution;\n"
"  float2 texture_dimensions;\n"
"};\n"
"\n"
"struct VS_Input\n"
"{\n"
"  float2 top_left:    INSTANCE_SIZE0;\n"
"  float2 bottom_right: INSTANCE_SIZE1;\n"
"\n"
"  float4 color:    INSTANCE_COLOR;\n"
"  float2 position: INSTANCE_POSITION;\n"
"\n"
"  float2 texture_top_left:     INSTANCE_UV0;\n"
"  float2 texture_bottom_right: INSTANCE_UV1;\n"
"\n"
"  // NOTE(antonio): synthetic\n"
"  uint vertex_id: SV_VertexID;\n"
"};\n"
"\n"
"struct PS_Input\n"
"{\n"
"  float4 vertex: SV_POSITION;\n"
"  float2 uv: TEXCOORD;\n"
"  float4 color: COLOR;\n"
"};\n"
"\n"
"Global_Data  global_data;\n"
"Texture2D    global_texture: register(t0);\n"
"SamplerState global_sampler: register(s0);\n"
"\n"
"PS_Input\n"
"VS_Main(VS_Input input)\n"
"{\n"
"  // NOTE(antonio): static vertex array that we can index into with our vertex ID\n"
"  static float2 vertices[] =\n"
"  {\n"
"    // NOTE(antonio): Bottom Left\n"
"    {-1.0f, -1.0f},\n"
"    // NOTE(antonio): Top Left\n"
"    {-1.0f, +1.0f},\n"
"    // NOTE(antonio): Bottom Right\n"
"    {+1.0f, -1.0f},\n"
"    // NOTE(antonio): Top Right\n"
"    {+1.0f, +1.0f},\n"
"  };\n"
"\n"
"  PS_Input output;\n"
"  float2 destination_half_size = (input.bottom_right - input.top_left) / 2;\n"
"  float2 destination_center = (input.top_left + input.bottom_right) / 2;\n"
"\n"
"  float2 destination_position =\n"
"    (vertices[input.vertex_id] * destination_half_size) + destination_center;\n"
"\n"
"  destination_position.xy += input.position.xy;\n"
"\n"
"  output.vertex = float4((2 * destination_position.x / global_data.resolution.x) - 1,\n"
"                         1 - (2 * destination_position.y / global_data.resolution.y),\n"
"                         0, 1);\n"
"\n"
"  float2 source_half_size = (input.texture_bottom_right - input.texture_top_left) / 2;\n"
"  float2 source_center = (input.texture_top_left + input.texture_bottom_right) / 2;\n"
"\n"
"  float2 source_position =\n"
"    ((vertices[input.vertex_id] * source_half_size) + source_center);\n"
"\n"
"  float texture_width = global_data.texture_dimensions.x;\n"
"  float texture_height = global_data.texture_dimensions.y;\n"
"\n"
"  output.uv = float2(source_position.x / texture_width,\n"
"                     source_position.y / texture_height);\n"
"\n"
"  output.color = input.color;\n"
"  return output;\n"
"}\n"
"\n"
"float4\n"
"PS_Main(PS_Input input): SV_Target\n"
"{\n"
"  float4 texture_sample = global_texture.Sample(global_sampler, input.uv);\n"
"  float4 out_color = texture_sample * input.color;\n"
"\n"
"  return out_color;\n"
"}"
);

global b32 global_running        = false;
global b32 global_window_resized = false;

internal String_Const_utf8
win32_read_clipboard_contents()
{
  String_Const_utf8 result = {};
  if (OpenClipboard(win32_global_state.window_handle))
  {
    b32 got_result = false;

    {
      HANDLE clip_data = GetClipboardData(CF_UNICODETEXT);
      if (clip_data != NULL)
      {
        utf16 *clip_data_utf16 = (utf16 *) GlobalLock(clip_data);
        if (clip_data_utf16 != NULL)
        {
          String_Const_utf16 clip_utf16 = string_const_utf16(clip_data_utf16);

          assert(!"unimplemented");

          got_result = true;
        }
        GlobalUnlock(clip_data);
      }
    }

    if (!got_result)
    {
      HANDLE clip_data = GetClipboardData(CF_TEXT);
      if (clip_data != 0)
      {
        char *clip_data_char = (char *) GlobalLock(clip_data);
        if (clip_data_char != 0)
        {
          String_Const_char clip_char = string_const_char(clip_data_char);

          assert(!"unimplemented");

          got_result = true;
        }
        GlobalUnlock(clip_data);
      }
    }

    CloseClipboard();
  }

  return(result);
}

internal LRESULT
win32_window_procedure(HWND window_handle, UINT message,
                       WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;

  switch (message)
  {
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    {
      if (wparam == VK_ESCAPE)
      {
        global_running = false;
      }
    } break;

    case WM_SIZE:
    {
      global_window_resized = true;
      result = DefWindowProc(window_handle, message, wparam, lparam);
    } break;

    case WM_QUIT:
    case WM_DESTROY:
    case WM_CLOSE:
    {
      global_running = false;
    } break;

    case WM_LBUTTONDOWN:
    {
    } break;

    default:
    {
      result = DefWindowProc(window_handle, message, wparam, lparam);
    } break;
  }

  return(result);
}

global u8 temp_arena_data[mb(4)];
global u8 default_font_data_static[mb(2)];

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

  rng_init();

  wchar_t _exe_file_path[MAX_PATH] = {};
  GetModuleFileNameW(NULL, _exe_file_path, array_count(_exe_file_path));

  String_Const_utf16 exe_file_path = string_const_utf16((utf16 *) _exe_file_path);
  unused(exe_file_path);

  SetCurrentDirectoryW((LPCWSTR) exe_file_path.str);

  String_Const_utf8 default_font_path = string_literal_init_type("C:/windows/fonts/arial.ttf", utf8);
  Arena arena = {temp_arena_data, array_count(temp_arena_data), 0, 1};

  File_Buffer arial_font = platform_open_and_read_entire_file(&arena, default_font_path.str, default_font_path.size);

  // NOTE(antonio): default font on Windows is Arial
  default_font = platform_open_and_read_entire_file(&arena, default_font_path.str, default_font_path.size);

  f32 default_font_heights[] = {14.0f, 24.0f};
  Font_Data font_data;

  font_initialize(&arena, &font_data, &arial_font, default_font_heights, array_count(default_font_heights));

  {
    WNDCLASSEXW window_class = {};

    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = win32_window_procedure;
    window_class.hInstance     = instance;
    window_class.hIcon         = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    window_class.lpszClassName = L"Windows Prototype";
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
                      L"Windows Prototype",
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

  // NOTE(antonio): initializing WinSock and TLS
  Socket tls_socket = {};
  {
    WSADATA winsock_metadata = {};

    i32 winsock_result = WSAStartup(MAKEWORD(2, 2), &winsock_metadata);
    assert((winsock_result == 0) && "expected winsock dll to load");

    u8 host_name[] = "finnhub.io";
    u16 port = 443;

#if 0
    addrinfo hints = {};
    {
      hints.ai_flags    = 0;
      hints.ai_family   = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
    }

    u8 host_name[] = "finnhub.io";
    u8 service[] = "http";

    winsock_result = getaddrinfo((PCSTR) host_name, (PCSTR) service, &hints, &addr_start);
    assert((winsock_result == 0) && "expected to get address info");

    addrinfo *cur_addr = NULL;
    b32 connected = false;

    for (cur_addr = addr_start;
         !connected && (cur_addr != NULL);
         cur_addr = cur_addr->ai_next)
    {
      win32_socket = socket(cur_addr->ai_family, cur_addr->ai_socktype, cur_addr->ai_protocol);

      assert((win32_socket != INVALID_SOCKET) && "expected to get valid socket");

      winsock_result = connect(win32_socket, cur_addr->ai_addr, (int) cur_addr->ai_addrlen);
      if (winsock_result == SOCKET_ERROR)
      {
        closesocket(win32_socket);
        win32_socket = INVALID_SOCKET;
      }
      else
      {
        // TODO(antonio): log the addrinfo?
        connected = true;
      }
    }

    assert(connected && "expected to be connected here");
    freeaddrinfo(addr_start);
#endif

    tls_socket.socket = socket(AF_INET, SOCK_STREAM, 0);

    u8 port_name[64] = {};
    stbsp_snprintf((char *) port_name, sizeof(port_name), "%u", port);

    // TODO(antonio): ansi versions are deprecated
    b32 connected = WSAConnectByNameA(tls_socket.socket,
                                      (char *) host_name,
                                      (char *) port_name,
                                      NULL, NULL, NULL, NULL, NULL, NULL);
    assert(connected && "expected connection");

    // NOTE(antonio): initialize schannel
    {
      SCHANNEL_CRED cred = {};
      {
          cred.dwVersion               = SCHANNEL_CRED_VERSION;
          cred.cCreds                  = 0;
          cred.paCred                  = NULL;
          cred.hRootStore              = NULL;
          cred.cSupportedAlgs          = 0;
          cred.palgSupportedAlgs       = NULL;
          cred.grbitEnabledProtocols   = SP_PROT_TLS1_2;
          cred.dwMinimumCipherStrength = 0;
          cred.dwMaximumCipherStrength = 0;
          // TODO(antonio): need to manage SCHANNEL sessions too :(
          //                this only lasts 10 hours
          cred.dwSessionLifespan       = 0;
          cred.dwFlags                 = SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_NO_DEFAULT_CREDS | SCH_USE_STRONG_CRYPTO;
          cred.dwCredFormat            = 0;
      };

      SECURITY_STATUS acquire_result =
        AcquireCredentialsHandleA(NULL,
                                  UNISP_NAME_A,
                                  SECPKG_CRED_OUTBOUND,
                                  NULL,
                                  &cred,
                                  NULL,
                                  NULL,
                                  &tls_socket.cred_handle,
                                  NULL);

      assert((acquire_result == SEC_E_OK) && "expected to get a credential handle");
    }

    // NOTE(antonio): perform tls handshake
    //                1) call InitializeSecurityContext to create/update schannel context
    //                2) when it returns SEC_E_OK - tls handshake completed
    //                3) when it returns SEC_I_INCOMPLETE_CREDENTIALS - server requests client certificate (TODO)
    //                4) when it returns SEC_I_CONTINUE_NEEDED - send token to server and read data
    //                5) when it returns SEC_E_INCOMPLETE_MESSAGE - need to read more data from server
    //                6) otherwise read data from server and go to step 1

    CtxtHandle* security_context = NULL;
    i32 result = 0;

    unused(result);

    for (;;)
    {
      SecBuffer in_buffers[2] = {};
      {
        in_buffers[0].BufferType = SECBUFFER_TOKEN;
        in_buffers[0].pvBuffer   = tls_socket.incoming;
        in_buffers[0].cbBuffer   = tls_socket.received;

        in_buffers[1].BufferType = SECBUFFER_EMPTY;
      }

      SecBuffer out_buffers[1] = {};
      {
        out_buffers[0].BufferType = SECBUFFER_TOKEN;
      }

      SecBufferDesc in_desc  = {SECBUFFER_VERSION, array_count(in_buffers),  in_buffers};
      SecBufferDesc out_desc = {SECBUFFER_VERSION, array_count(out_buffers), out_buffers};

      DWORD security_init_flags = ISC_REQ_ALLOCATE_MEMORY    |
                                  ISC_REQ_CONFIDENTIALITY    |
                                  ISC_REQ_USE_SUPPLIED_CREDS |
                                  ISC_REQ_REPLAY_DETECT      |
                                  ISC_REQ_SEQUENCE_DETECT    |
                                  ISC_REQ_STREAM;

      SECURITY_STATUS sec = InitializeSecurityContextA(&tls_socket.cred_handle,
                                                       security_context,
                                                       security_context ? NULL : (SEC_CHAR *) host_name,
                                                       security_init_flags,
                                                       0,
                                                       0,
                                                       security_context ? &in_desc : NULL,
                                                       0,
                                                       security_context ? NULL : &tls_socket.security_context,
                                                       &out_desc,
                                                       &security_init_flags,
                                                       NULL);

      unused(sec);

      // NOTE(antonio): after first call to InitializeSecurityContextA, context will be available and can be reused 
      security_context = &tls_socket.security_context;

      // NOTE(antonio):
      // used to indicated unprocessed byte count
      // moving memory from buffer that still needs to be processed to incoming
      //
      // 0                         received
      // <.............************            >
      // . - processed
      // * - unprocessed
      //   - unused
      //
      // post-operation
      // <************                         >
      if (in_buffers[1].BufferType == SECBUFFER_EXTRA)
      {
        move_memory_block(tls_socket.incoming,
                          tls_socket.incoming + (tls_socket.received - in_buffers[1].cbBuffer),
                          in_buffers[1].cbBuffer);
        tls_socket.received = in_buffers[1].cbBuffer;
      }
      else
      {
        tls_socket.received = 0;
      }

      if (sec == SEC_E_OK)
      {
        // NOTE(antonio): tls handshake completed
        break;
      }
      else if (sec == SEC_I_INCOMPLETE_CREDENTIALS)
      {
        // NOTE(antonio): server asked for client certificate, not supported here
        result = -1;
        assert(!"unimplemented");
        break;
      }
      else if (sec == SEC_I_CONTINUE_NEEDED)
      {
        // NOTE(antonio): need to send output token to server
        u8 *output_token = (u8 *) out_buffers[0].pvBuffer;
        i32 token_size = out_buffers[0].cbBuffer;

        while (token_size != 0)
        {
          i32 bytes_sent = send(tls_socket.socket, (char *) output_token, token_size, 0);

          if (bytes_sent <= 0)
          {
            break;
          }

          token_size   -= bytes_sent;
          output_token += bytes_sent;
        }

        FreeContextBuffer(out_buffers[0].pvBuffer);

        if (token_size != 0)
        {
          result = -1;
          assert(!"failed to fully send data to server");
          break;
        }
      }
      else if (sec != SEC_E_INCOMPLETE_MESSAGE)
      {
        // NOTE(antonio):
        // SEC_E_CERT_EXPIRED    - certificate expired or revoked
        // SEC_E_WRONG_PRINCIPAL - bad hostname
        // SEC_E_UNTRUSTED_ROOT  - cannot vertify CA chain
        // SEC_E_ILLEGAL_MESSAGE / SEC_E_ALGORITHM_MISMATCH - cannot negotiate crypto algorithms
        result = -1;
          assert(!"unimplemented");
        break;
      }

      // read more data from server when possible
      if (tls_socket.received == sizeof(tls_socket.incoming))
      {
        // server is sending too much data instead of proper handshake?
        result = -1;
        break;
      }

      i32 bytes_received = recv(tls_socket.socket,
                                (char *) (tls_socket.incoming + tls_socket.received),
                                sizeof(tls_socket.incoming) - tls_socket.received,
                                0);
      if (bytes_received == 0)
      {
        // server disconnected socket
        result = 0;
        break;
      }
      else if (bytes_received < 0)
      {
        // socket error
        result = -1;
        break;
      }

      tls_socket.received += bytes_received;
    }

#if 0
    // NOTE(antonio): connection has been established
    //                sending websocket handshake
    {
      u8 header[512]  = {};

      u8 header_bytes[16] = {};
      rng_fill_buffer(header_bytes, sizeof(header_bytes));

      u8 header_key[32] = {};
      base64_encode(header_bytes, sizeof(header_bytes), header_key);

      u8 path[]  = "";
      u8 query[] = "";

      u8 origin_name[] = "localhost:8000";
      u8 api_key[] = "cgtgdupr01qoiqvp24t0cgtgdupr01qoiqvp24tg"; 

      i32 header_length =
        stbsp_snprintf((char *) header, array_count(header),
                       "GET https://ws.%s/%s%s HTTP/1.1\r\n"
                       "Host: ws.%s\r\n"
                       "Upgrade: websocket\r\n"
                       "Connection: keep-alive, Upgrade\r\n"
                       "Sec-WebSocket-Key: %s\r\n"
                       "Origin: %s\r\n"
                       "Sec-WebSocket-Protocol: chat, superchat\r\n"
                       "Sec-WebSocket-Version: 13\r\n"
                       "X-Finnhub-Token: %s\r\n"
                       "\r\n\r\n",
                       (char *) host_name,
                       (char *) path,
                       (char *) query,
                       (char *) host_name,
                       (char *) header_key,
                       (char *) origin_name,
                       (char *) api_key);

      i32 bytes_sent = send(win32_socket, (const char *) header, header_length, 0);
      assert((bytes_sent != SOCKET_ERROR) && (bytes_sent == header_length) && "did not send the expected number of bytes");
    }

    // NOTE(antonio): receiving response
    {
      u8 receive_buffer[1024] = {};
      i32 receive_length = 0;

      do {
        receive_length = recv(win32_socket, (char *) receive_buffer, array_count(receive_buffer) - 1, 0);
      }
      while (receive_length > 0);

      OutputDebugStringA((char *) receive_buffer);
    }
#endif
  }

  if (ShowWindow(win32_global_state.window_handle, SW_NORMAL) && UpdateWindow(win32_global_state.window_handle))
  {
    // NOTE(antonio): initializing Direct3D 11
    ID3D11Device1 *device = NULL;
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

      result = base_device->QueryInterface(__uuidof(ID3D11Device1), (void **) &device);
      assert(SUCCEEDED(result));
      base_device->Release();

      result = base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **) &device_context);
      assert(SUCCEEDED(result));
      base_device_context->Release();
    }

#if !SHIP_MODE
    ID3D11Debug *debug = NULL;
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
        assert(SUCCEEDED(result));

        IDXGIAdapter *dxgi_adapter = NULL;
        result = dxgi_device->GetAdapter(&dxgi_adapter);
        assert(SUCCEEDED(result));
        dxgi_device->Release();

        DXGI_ADAPTER_DESC adapter_description = {};
        dxgi_adapter->GetDesc(&adapter_description);

        OutputDebugStringW(L"Graphics Device: ");
        OutputDebugStringW(adapter_description.Description);

        result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void **) &dxgi_factory);
        assert(SUCCEEDED(result));
        dxgi_adapter->Release();
      }

      DXGI_SWAP_CHAIN_DESC1 swap_chain_description = {};

      swap_chain_description.Width  = 0;  // use window width
      swap_chain_description.Height = 0;  // use window height
      swap_chain_description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

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
      assert(SUCCEEDED(result));

      dxgi_factory->Release();
    }

    ID3D11RenderTargetView *frame_buffer_view = NULL;
    {
      ID3D11Texture2D *frame_buffer = NULL;
      HRESULT result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &frame_buffer);
      assert(SUCCEEDED(result));

      result = device->CreateRenderTargetView(frame_buffer, 0, &frame_buffer_view);
      assert(SUCCEEDED(result));
      frame_buffer->Release();
    }

    ID3DBlob *vertex_shader_blob = NULL;
    ID3D11VertexShader *vertex_shader = NULL;
    {
      ID3DBlob *shader_compile_errors_blob = NULL;
      HRESULT result = D3DCompile(shader.str, shader.size, NULL, NULL, NULL,
                                  "VS_Main", "vs_5_0", 0, 0, &vertex_shader_blob, &shader_compile_errors_blob);
      if (FAILED(result))
      {
        String_Const_char error_string = {};

        if (shader_compile_errors_blob)
        {
          error_string = {(char *) shader_compile_errors_blob->GetBufferPointer(), shader_compile_errors_blob->GetBufferSize()};
          shader_compile_errors_blob->Release();
        }
        MessageBoxA(0, error_string.str, "Shader Compiler Error", MB_ICONERROR | MB_OK);

        return(1);
      }

      result = device->CreateVertexShader(vertex_shader_blob->GetBufferPointer(),
                                          vertex_shader_blob->GetBufferSize(),
                                          NULL, &vertex_shader);
      assert(SUCCEEDED(result));
    }

    ID3D11PixelShader *pixel_shader = NULL;
    {
      ID3DBlob *pixel_shader_blob = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      HRESULT result = D3DCompile(shader.str, shader.size, NULL, NULL, NULL,
                                  "PS_Main", "ps_5_0", 0, 0, &pixel_shader_blob, &shader_compile_errors_blob);
      if (FAILED(result))
      {
        String_Const_char error_string = {};

        if (shader_compile_errors_blob)
        {
          error_string = {(char *) shader_compile_errors_blob->GetBufferPointer(), shader_compile_errors_blob->GetBufferSize()};
          shader_compile_errors_blob->Release();
        }

        MessageBoxA(0, error_string.str, "Shader Compiler Error", MB_ICONERROR | MB_OK);

        return(1);
      }

      result = device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(),
                                         pixel_shader_blob->GetBufferSize(), NULL,
                                         &pixel_shader);
      assert(SUCCEEDED(result));

      pixel_shader_blob->Release();
    }

    ID3D11InputLayout *input_layout = NULL;
    {
      D3D11_INPUT_ELEMENT_DESC input_element_description[] =
      {
#define INSTANCE_BUFFER_SLOT 0
        // NOTE(antonio): instance buffer data
        {"INSTANCE_SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_SIZE", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},

        {"INSTANCE_COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       INSTANCE_BUFFER_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},

        {"INSTANCE_UV", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"INSTANCE_UV", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
      };

      HRESULT result = device->CreateInputLayout(input_element_description, array_count(input_element_description),
                                                 vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize(),
                                                 &input_layout);
      assert(SUCCEEDED(result));
      vertex_shader_blob->Release();
    }

    ID3D11Buffer *instance_buffer = NULL;
    {
      D3D11_BUFFER_DESC instance_buffer_description = {};

      instance_buffer_description.ByteWidth      = 512;
      instance_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
      instance_buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
      instance_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      HRESULT result = device->CreateBuffer(&instance_buffer_description, NULL, &instance_buffer);
      assert(SUCCEEDED(result));
    }

    ID3D11SamplerState *sampler_state = NULL;
    {
      D3D11_SAMPLER_DESC sampler_description = {};

      sampler_description.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
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
      assert(SUCCEEDED(result));
    }

    ID3D11BlendState *transparent_blend_state = NULL;
    {
      D3D11_BLEND_DESC blend_description = {};

      blend_description.AlphaToCoverageEnable                 = FALSE;
      blend_description.IndependentBlendEnable                = FALSE;        
      blend_description.RenderTarget[0].BlendEnable           = TRUE;
      blend_description.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
      blend_description.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      blend_description.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
      blend_description.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
      blend_description.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_DEST_ALPHA;
      blend_description.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      blend_description.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      HRESULT result = device->CreateBlendState(&blend_description, &transparent_blend_state);
      assert(SUCCEEDED(result));
    }

    ID3D11RasterizerState* rasterizer_state = {};
    {
      D3D11_RASTERIZER_DESC rasterizer_description = {};

      rasterizer_description.FillMode              = D3D11_FILL_SOLID;
      rasterizer_description.CullMode              = D3D11_CULL_NONE;
      rasterizer_description.FrontCounterClockwise = TRUE;
      rasterizer_description.ScissorEnable         = TRUE;

      HRESULT result = device->CreateRasterizerState(&rasterizer_description, &rasterizer_state);
      assert(SUCCEEDED(result));
    }

    ID3D11ShaderResourceView *texture_view = NULL;
    {
      D3D11_TEXTURE2D_DESC texture_description = {};

      texture_description.Width            = font_data.bitmap.height;
      texture_description.Height           = font_data.bitmap.width;
      texture_description.MipLevels        = 1;
      texture_description.ArraySize        = 1;
      texture_description.Format           = DXGI_FORMAT_R8_UNORM;
      texture_description.SampleDesc.Count = 1;
      texture_description.Usage            = D3D11_USAGE_DEFAULT;
      texture_description.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
      texture_description.CPUAccessFlags   = 0;

      D3D11_SUBRESOURCE_DATA subresource = {};

      subresource.pSysMem = font_data.bitmap.alpha;
      subresource.SysMemPitch = texture_description.Width * 1;
      subresource.SysMemSlicePitch = 0;

      ID3D11Texture2D *texture_2d = NULL;
      HRESULT result = device->CreateTexture2D(&texture_description, &subresource, &texture_2d);

      assert(SUCCEEDED(result));

      D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};

      shader_resource_view_description.Format                    = DXGI_FORMAT_R8_UNORM;
      shader_resource_view_description.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
      shader_resource_view_description.Texture2D.MipLevels       = texture_description.MipLevels;
      shader_resource_view_description.Texture2D.MostDetailedMip = 0;

      result = device->CreateShaderResourceView(texture_2d, &shader_resource_view_description, &texture_view);
      texture_2d->Release();

      assert(SUCCEEDED(result));
    }

    ID3D11Buffer *constant_buffer = NULL;
    {
      D3D11_BUFFER_DESC constant_buffer_description = {};

      // NOTE(antonio): ByteWidth must be a multiple of 16, per the docs
      constant_buffer_description.ByteWidth      = (sizeof(f32) + 0xf) & 0xfffffff0;
      constant_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
      constant_buffer_description.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
      constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      HRESULT result = device->CreateBuffer(&constant_buffer_description, NULL, &constant_buffer);
      assert(SUCCEEDED(result));
    }

    ID3D11DepthStencilState *depth_stencil_state = NULL;
    {
      D3D11_DEPTH_STENCIL_DESC depth_stencil_state_description = {};

      depth_stencil_state_description.DepthEnable                  = FALSE;
      depth_stencil_state_description.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
      depth_stencil_state_description.DepthFunc                    = D3D11_COMPARISON_ALWAYS;
      depth_stencil_state_description.StencilEnable                = FALSE;
      depth_stencil_state_description.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
      depth_stencil_state_description.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
      depth_stencil_state_description.BackFace                     = depth_stencil_state_description.FrontFace;

      HRESULT result = device->CreateDepthStencilState(&depth_stencil_state_description, &depth_stencil_state);
      assert(SUCCEEDED(result));
    }

    Render_Context render_context = {};
    {
      render_context.swap_chain     = swap_chain;
      render_context.device         = device;
      render_context.device_context = device_context;
    };

    global_running = true;
    global_window_resized = true;

    while (global_running)
    {
      MSG message;

      while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }

      FLOAT background_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
      device_context->ClearRenderTargetView(frame_buffer_view, background_color);
      device_context->OMSetRenderTargets(1, &frame_buffer_view, NULL);

      Rect_f32 client_rect = render_get_client_rect();

      D3D11_VIEWPORT viewport =
      {
        0.0f, 0.0f,
        client_rect.x1, client_rect.y1,
        0.0f, 1.0f
      };

      device_context->RSSetViewports(1, &viewport);

      device_context->IASetInputLayout(input_layout);
      device_context->IASetVertexBuffers(0, 1, &instance_buffer, 0, 0);
      device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

      device_context->VSSetShader(vertex_shader, NULL, 0);
      // device_context->VSSetConstantBuffers(0, 1, vertex_constant_buffer);

      device_context->PSSetShader(pixel_shader, NULL, 0);
      device_context->PSSetShaderResources(0, 1, &texture_view);
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

      swap_chain->Present(1, 0);
    }
  }

  return(0);
}
