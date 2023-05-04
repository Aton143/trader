#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#define UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include <memoryapi.h>
#include <sysinfoapi.h>
#include <fileapi.h>
#include <intrin.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#include <stdio.h>

#define TLS_MAX_PACKET_SIZE (16384 + 512)

#include <d3d11_1.h>
#include <d3dcompiler.h>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "..\trader.h"

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

  utf16 _exe_file_path[MAX_PATH] = {};
  GetModuleFileNameW(NULL, _exe_file_path, array_count(_exe_file_path));

  int i; i = array_count(win32_global_state.changed_files[0]);

  String_Const_utf16 exe_file_path = string_const_utf16((utf16 *) _exe_file_path);
  unused(exe_file_path);

  SetCurrentDirectoryW((LPCWSTR) exe_file_path.str);

  Arena global_arena = arena_alloc(global_memory_size, 4, (void *) global_memory_start_addr);
  Arena render_data  = arena_alloc(render_data_size, 1, NULL);

  Asset_Node *asset_pool_start = (Asset_Node *) VirtualAlloc(NULL, mb(1), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  u64 asset_count = mb(1) / sizeof(*asset_pool_start);
  for (u64 asset_index = 0;
       asset_index < asset_count - 1;
       ++asset_index)
  {
    asset_pool_start[asset_index].next = asset_pool_start + (asset_index + 1);
  }

  global_asset_pool.free_list_head = asset_pool_start;
  win32_global_state.temp_arena    = arena_alloc(global_asset_pool_temp_arena_size, 4, NULL);

  HANDLE iocp_handle = INVALID_HANDLE_VALUE;
  {
    // TODO(antonio): use completion key to distinguish handles?
    iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (iocp_handle == INVALID_HANDLE_VALUE)
    {
      assert(!"could not create an I/O completion port");
    }

    win32_global_state.notify_iocp = iocp_handle;
  }

  String_Const_utf8 notify_dir = string_literal_init_type("..\\src\\platform_win32\\", utf8);

  platform_push_notify_dir(notify_dir.str, notify_dir.size);
  platform_start_collect_notifications();

  String_Const_utf8 default_font_path = string_literal_init_type("C:/windows/fonts/arial.ttf", utf8);

  File_Buffer arial_font = platform_open_and_read_entire_file(&global_arena, default_font_path.str, default_font_path.size);

  // NOTE(antonio): default font on Windows is Arial
  default_font = platform_open_and_read_entire_file(&global_arena, default_font_path.str, default_font_path.size);

  f32 default_font_heights[] = {8.0f, 14.0f, 24.0f, 30.0f};

  Texture_Atlas *atlas = push_struct_zero(&global_arena, Texture_Atlas);
  render_atlas_initialize(&global_arena,
                          atlas,
                          &arial_font,
                          default_font_heights,
                          array_count(default_font_heights),
                          512, 512);

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

#if 0
  Network_State network_state = {};
  network_startup(&network_state);

  u8 _host_name[] = "finnhub.io";
  String_Const_utf8 host_name = string_literal_init(_host_name);

  u8 _query_path[] = "";
  String_Const_utf8 query_path = string_literal_init(_query_path);

  u16 port = 443;

  Socket tls_socket;
  network_connect(&network_state, &tls_socket, host_name, port);

  u8 _request_header[1024] = {};
  Buffer request_header = buffer_from_fixed_size(_request_header);

  request_header.used =
    (u64) stbsp_snprintf((char *) request_header.data,
                         (int) request_header.size,
                         "GET /api/v1/search?q=apple HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Accept: */*\r\n"
                         "Connection: keep-alive\r\n",
                         (char *) host_name.str);

  network_send_simple(&network_state, &tls_socket, &request_header);

  u8 _receive_buffer[1024] = {};
  Buffer receive_buffer = buffer_from_fixed_size(_receive_buffer);

  network_receive_simple(&network_state, &tls_socket, &receive_buffer);
#endif

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

    String_Const_utf8 shader_source_path = string_literal_init_type("..\\src\\platform_win32\\shaders.hlsl", utf8);
    File_Buffer shader_source =
      platform_open_and_read_entire_file(&global_arena,
                                         shader_source_path.str,
                                         shader_source_path.size);

    ID3DBlob *vertex_shader_blob = NULL;
    ID3D11VertexShader *vertex_shader = NULL;
    {
      ID3DBlob *shader_compile_errors_blob = NULL;
      HRESULT result = D3DCompile(shader_source.data, shader_source.size, NULL, NULL, NULL,
                                  "VS_Main", "vs_5_0", 0, 0, &vertex_shader_blob, &shader_compile_errors_blob);
      if (FAILED(result))
      {
        String_Const_char error_string = {};

        if (shader_compile_errors_blob)
        {
          error_string =
          {
            (char *) shader_compile_errors_blob->GetBufferPointer(),
            shader_compile_errors_blob->GetBufferSize()
          };
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
      ID3DBlob *pixel_shader_blob          = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      HRESULT result = D3DCompile(shader_source.data, shader_source.size, NULL, NULL, NULL,
                                  "PS_Main", "ps_5_0", 0, 0, &pixel_shader_blob, &shader_compile_errors_blob);
      if (FAILED(result))
      {
        String_Const_char error_string = {};

        if (shader_compile_errors_blob)
        {
          error_string =
          {
            (char *) shader_compile_errors_blob->GetBufferPointer(),
            shader_compile_errors_blob->GetBufferSize()
          };
          shader_compile_errors_blob->Release();
        }

        MessageBoxA(0, error_string.str, "Shader Compiler Error", MB_ICONERROR | MB_OK);

        return(1);
      }

      result = device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(),
                                         pixel_shader_blob->GetBufferSize(),
                                         NULL,
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
        {
          "INSTANCE_SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          0, D3D11_INPUT_PER_INSTANCE_DATA, 1
        },

        {
          "INSTANCE_SIZE", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        },

        {
          "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        },

        {
          "INSTANCE_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},

        {"INSTANCE_UV", 0, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
        },

        {
          "INSTANCE_UV", 1, DXGI_FORMAT_R32G32_FLOAT, INSTANCE_BUFFER_SLOT,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
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

      instance_buffer_description.ByteWidth      = (u32) render_data.size;
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

      assert(SUCCEEDED(result));

      D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};
      {
        shader_resource_view_description.Format                    = DXGI_FORMAT_R8_UNORM;
        shader_resource_view_description.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_description.Texture2D.MipLevels       = texture_description.MipLevels;
        shader_resource_view_description.Texture2D.MostDetailedMip = 0;
      }

      result = device->CreateShaderResourceView(texture_2d, &shader_resource_view_description, &font_texture_view);
      texture_2d->Release();

      assert(SUCCEEDED(result));
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

    {
      win32_global_state.render_context.swap_chain     = swap_chain;
      win32_global_state.render_context.device         = device;
      win32_global_state.render_context.device_context = device_context;

      win32_global_state.render_context.render_data    = render_data;
    };

    global_running = true;
    global_window_resized = true;

    {
      Instance_Buffer_Element *element = push_struct(&win32_global_state.render_context.render_data, Instance_Buffer_Element);

      element->size.x0 = 0;
      element->size.y0 = 0;
      element->size.x1 = atlas->bitmap.width;
      element->size.y1 = atlas->bitmap.height;

      element->color.r = 1.0f;
      element->color.g = 1.0f;
      element->color.b = 1.0f;
      element->color.a = 1.0f;

      element->pos.x   = 0.0f;
      element->pos.y   = 0.0f;
      element->pos.z   = 0.0f;

      element->uv.x0   = 0.0f;
      element->uv.y0   = 0.0f;
      element->uv.x1   = atlas->bitmap.width;
      element->uv.y1   = atlas->bitmap.height;
    }

    while (global_running)
    {
      MSG message;

      while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }

      if (global_window_resized)
      {
        global_window_resized = false;

        device_context->OMSetRenderTargets(0, 0, 0);
        frame_buffer_view->Release();

        HRESULT result = swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        // assert(SUCCEEDED(result));

        ID3D11Texture2D* frame_buffer = NULL;
        result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &frame_buffer);
        // assert(SUCCEEDED(result));

        result = device->CreateRenderTargetView(frame_buffer, NULL, &frame_buffer_view);
        // assert(SUCCEEDED(result));
        frame_buffer->Release();
      }

      platform_collect_notifications();

      FLOAT background_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      device_context->ClearRenderTargetView(frame_buffer_view, background_color);
      device_context->OMSetRenderTargets(1, &frame_buffer_view, NULL);

      Rect_f32 client_rect = render_get_client_rect();

      {
        Constant_Buffer constant_buffer_items = {};
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

      device_context->VSSetShader(vertex_shader, NULL, 0);
      device_context->VSSetConstantBuffers(0, 1, &constant_buffer);

      device_context->PSSetShader(pixel_shader, NULL, 0);
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

      u32 draw_call_count = (u32) (win32_global_state.render_context.render_data.used / sizeof(Instance_Buffer_Element));
      device_context->DrawInstanced(4, draw_call_count, 0, 0);

      swap_chain->Present(1, 0);
    }
  }

  return(0);
}
