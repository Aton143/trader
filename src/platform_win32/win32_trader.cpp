#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NO_MIN_MAX
#define UNICODE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NO_VA_START_VALIDATION
#define OEMRESOURCE

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

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

THREAD_RETURN render_thread_proc(void *_args)
{
  uintptr_t *args = (uintptr_t *) _args;

  Thread_Context *thread_context = &thread_contexts[*args++];
  Arena *temp_arena = get_temp_arena(thread_context);

  Global_Platform_State *global_state = platform_get_global_state();
  Common_Render_Context *common_render = render_get_common_context();
  Render_Context        *render = render_get_context();

  Ring_Buffer *command_queue = &common_render->command_queue;

  ID3D11InputLayout *input_layouts[3]  = {};
  ID3D11Buffer      *vertex_buffers[3] = {};

  Arena _render_thread_arena = arena_alloc(1024 * sizeof(Render_Command), 1, NULL);
  Arena *render_thread_arena = &_render_thread_arena;

  ID3D11RenderTargetView *frame_buffer_view = NULL;
  {
    ID3D11Texture2D *frame_buffer = NULL;
    HRESULT result = render->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &frame_buffer);
    expect(SUCCEEDED(result));

    result = render->device->CreateRenderTargetView(frame_buffer, 0, &frame_buffer_view);
    expect(SUCCEEDED(result));
    frame_buffer->Release();
  }

  {
    String_Const_utf8 dummy_shader_path = scu8l("..\\src\\platform_win32\\input_layout_shaders.hlsl");
    File_Buffer input_layout_source =
      platform_open_and_read_entire_file(temp_arena,
                                         dummy_shader_path.str,
                                         dummy_shader_path.size);

    char *vertex_mains[3] = {"VS_Main_16", "VS_Main_32", "VS_Main_64"};

    for (u32 layout_index = 0;
         layout_index < array_count(input_layouts);
         ++layout_index)
    {
      {
        ID3D11VertexShader *dummy_vertex_shader = NULL;
        ID3DBlob *compiled_shader_blob =
          (ID3DBlob *) render_compile_vertex_shader(&input_layout_source, vertex_mains[layout_index], &dummy_vertex_shader);

        D3D11_INPUT_ELEMENT_DESC input_element_description[] =
        {
          {
            "IN", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
            D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
          }, {
            "IN", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
            D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
          }, {
            "IN", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
            D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
          }, {
            "IN", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
            D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
          },
        };

        HRESULT result = render->device->CreateInputLayout(input_element_description,
                                                           1 << (layout_index),
                                                           compiled_shader_blob->GetBufferPointer(),
                                                           compiled_shader_blob->GetBufferSize(),
                                                           &input_layouts[layout_index]);
        expect(SUCCEEDED(result));
        compiled_shader_blob->Release();
        dummy_vertex_shader->Release();
      }

      {
        D3D11_BUFFER_DESC vertex_buffer_description = {};

        vertex_buffer_description.ByteWidth      = (u32) kb(16);
        vertex_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
        vertex_buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        vertex_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT result = render->device->CreateBuffer(&vertex_buffer_description, NULL, &vertex_buffers[layout_index]);
        expect(SUCCEEDED(result));
      }
    }
  }

  ID3D11Buffer *constant_buffer = NULL;
  {
    D3D11_BUFFER_DESC constant_buffer_description = {};

    // NOTE(antonio): ByteWidth must be a multiple of 16, per the docs
    constant_buffer_description.ByteWidth      = (member_size(RCK_Draw, constant_buffer_data) + 0xf) & 0xfffffff0;
    constant_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
    constant_buffer_description.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT result = render->device->CreateBuffer(&constant_buffer_description, NULL, &constant_buffer);
    expect(SUCCEEDED(result));
  }

  ID3D11SamplerState *sampler_state = NULL;
  {
    D3D11_SAMPLER_DESC sampler_description = {};

    sampler_description.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_description.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_description.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_description.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_description.BorderColor[0] = 1.0f;
    sampler_description.BorderColor[1] = 1.0f;
    sampler_description.BorderColor[2] = 1.0f;
    sampler_description.BorderColor[3] = 1.0f;
    sampler_description.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_description.MinLOD         = 0;
    sampler_description.MaxLOD         = D3D11_FLOAT32_MAX;

    HRESULT result = render->device->CreateSamplerState(&sampler_description, &sampler_state);
    expect(SUCCEEDED(result));
  }

  ID3D11BlendState *pma_blend_state = NULL;
  f32 blend_factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
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

    HRESULT result = render->device->CreateBlendState(&blend_description, &pma_blend_state);
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

    HRESULT result = render->device->CreateDepthStencilState(&depth_stencil_state_description, &depth_stencil_state);
    expect(SUCCEEDED(result));
  }

  ID3D11RasterizerState* rasterizer_state = {};
  {
    D3D11_RASTERIZER_DESC rasterizer_description = {};

    rasterizer_description.FillMode              = D3D11_FILL_SOLID;
    rasterizer_description.CullMode              = D3D11_CULL_NONE;
    rasterizer_description.FrontCounterClockwise = TRUE;
    rasterizer_description.ScissorEnable         = TRUE;

    HRESULT result = render->device->CreateRasterizerState(&rasterizer_description, &rasterizer_state);
    expect(SUCCEEDED(result));
  }

  ID3D11Texture2D        *depth_stencil_texture = NULL;
  ID3D11DepthStencilView *depth_stencil_view    = NULL;

  *command_queue = ring_buffer_make(render_thread_arena, render_thread_arena->size);

  u32 buffer_positions[rck_count] = {};

  for (;;)
  {
    while (!global_state->render_thread_can_start_processing);

    while (!global_state->main_thread_done_submitting)
    {
      while (command_queue->read != command_queue->write)
      {
        Render_Command command;
        ring_buffer_pop_and_put(command_queue, &command, sizeof(Render_Command));

        switch (command.kind)
        {
          case rck_draw:
          {
            RCK_Draw *draw = &command.draw;

            D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
            {
              render->device_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_buffer);
              copy_memory_block(mapped_buffer.pData, draw->constant_buffer_data, sizeof(draw->constant_buffer_data));
              render->device_context->Unmap(constant_buffer, 0);
            }

            u32 input_layout_size  = power_of_2_ceil32(draw->per_vertex_size);
            u32 input_layout_index = first_msb_pos32(input_layout_size) - 4;
            expect(is_between_inclusive(0, input_layout_index, 2));

            u32 offsets = 0;
            render->device_context->IASetInputLayout(input_layouts[input_layout_index]);
            render->device_context->IASetVertexBuffers(0,
                                                       1,
                                                       &vertex_buffers[input_layout_index],
                                                       &draw->per_vertex_size,
                                                       &offsets);

            render->device_context->VSSetShader(draw->vertex_shader.shader, NULL, 0);
            render->device_context->PSSetShader(draw->pixel_shader.shader, NULL, 0);
            render->device_context->PSSetShaderResources(0,
                                                         array_count(draw->textures),
                                                         (ID3D11ShaderResourceView **) draw->textures);
            render->device_context->PSSetSamplers(0, 1, &sampler_state);

            render->device_context->GSSetShader(NULL, NULL, 0);
            render->device_context->HSSetShader(NULL, NULL, 0);
            render->device_context->DSSetShader(NULL, NULL, 0);
            render->device_context->CSSetShader(NULL, NULL, 0);

            render->device_context->OMSetBlendState(pma_blend_state, blend_factor, 0xffffffff);
            render->device_context->OMSetDepthStencilState(depth_stencil_state, 0);
          } break;

          case rck_clear:
          {
            RCK_Clear *clear = &command.clear;

            RGBA_f32  *background_color  = &clear->background_color;

            if (clear->clear_render_target)
            {
              render->device_context->ClearRenderTargetView(frame_buffer_view, (FLOAT *) background_color);
            }

            if (clear->clear_depth_stencil)
            {
              render->device_context->ClearDepthStencilView(depth_stencil_view,
                                                            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                                            1.0f,
                                                            0);
            }
          } break;

          case rck_resize:
          {

            Rect_f32 client_rect = render_get_client_rect();

            render->device_context->OMSetRenderTargets(0, 0, 0);
            frame_buffer_view->Release();

            HRESULT result = render->swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

            ID3D11Texture2D* new_frame_buffer = NULL;
            result = render->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &new_frame_buffer);
            result = render->device->CreateRenderTargetView(new_frame_buffer, NULL, &frame_buffer_view);

            safe_release(new_frame_buffer);

            safe_release(depth_stencil_texture);
            depth_stencil_texture = NULL;

            {
              D3D11_TEXTURE2D_DESC depth_stencil_texture_desc = {};

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

              result = render->device->CreateTexture2D(&depth_stencil_texture_desc, NULL, &depth_stencil_texture);
            }

            safe_release(depth_stencil_view);
            depth_stencil_view = NULL;
            {
              D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};

              depth_stencil_view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
              depth_stencil_view_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
              depth_stencil_view_desc.Texture2D.MipSlice = 0;

              result = render->device->CreateDepthStencilView(depth_stencil_texture,
                                                      &depth_stencil_view_desc,
                                                      &depth_stencil_view);
              // expect(SUCCEEDED(result));
            } 
          } break;

          default:
          {
            expect_message(false, "Either you didn't mean to use that kind of command or you're a dumb fuck!");
          }
        }
      }
    }

    zero_struct(buffer_positions);
    ring_buffer_reset(command_queue);

    global_state->render_thread_done_processing = true;
  }
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

  Global_Platform_State *global_state  = platform_get_global_state();
  Arena                 *global_arena  = platform_get_global_arena();
  Render_Context        *render        = render_get_context();
  Common_Render_Context *common_render = render_get_common_context();
  Texture_Atlas         *atlas         = common_render->atlas;

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

    global_state->notify_iocp = iocp_handle;
  }

  String_Const_utf8 notify_dir = string_literal_init_type("..\\src\\platform_win32\\", utf8);

  platform_push_notify_dir(notify_dir.str, notify_dir.size);
  platform_start_collect_notifications();

  File_Buffer font_data = platform_open_and_read_entire_file(global_arena,
                                                             default_font_path.str,
                                                             default_font_path.size);

  // NOTE(antonio): default font on Windows is Arial
  common_render->default_font =
    platform_open_and_read_entire_file(global_arena,
                                       default_font_path.str,
                                       default_font_path.size);

  render_atlas_initialize(global_arena,
                          atlas,
                          &font_data,
                          (f32 *) default_font_heights,
                          array_count(default_font_heights),
                          512, 512);

  String_Const_utf8 wingding = scu8l("C:\\windows\\fonts\\wingding.ttf");
  font_data = platform_open_and_read_entire_file(global_arena, wingding.str, wingding.size);

  render->wingding = push_struct(global_arena, Texture_Atlas);
  render_atlas_initialize(global_arena,
                          render->wingding,
                          &font_data,
                          (f32 *) default_font_heights,
                          array_count(default_font_heights),
                          512, 512);

  Bitmap skybox_bitmaps[6];
  win32_load_skybox(skybox_bitmaps);

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

    global_state->window_handle =
      CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
                      window_class.lpszClassName,
                      L"trader",
                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT,
                      NULL, NULL,
                      instance, NULL);

    if (!global_state->window_handle)
    {
      MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
      return GetLastError();
    }
  }

  // NOTE(antonio): ****RAWINPUT****
  // https://www.asciiart.eu/animals/reptiles/dinosaurs
  {
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((u16) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((u16) 0x02)
#endif

    local_persist RAWINPUTDEVICE raw_input_registrations[] =
    {
      {
        HID_USAGE_PAGE_GENERIC,
        HID_USAGE_GENERIC_MOUSE,
        RIDEV_INPUTSINK,
        global_state->window_handle,
      }
    };

    u32 count = array_count(raw_input_registrations);
    if (!RegisterRawInputDevices(raw_input_registrations, count, sizeof(raw_input_registrations[0])))
    {
      platform_debug_print_system_error();
    }
  }

  if (ShowWindow(global_state->window_handle, SW_NORMAL) && UpdateWindow(global_state->window_handle))
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

      swap_chain_description.Width  = 0; // use window width
      swap_chain_description.Height = 0; // use window height
      swap_chain_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

      swap_chain_description.SampleDesc.Count   = 1;
      swap_chain_description.SampleDesc.Quality = 0;

      swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swap_chain_description.BufferCount = 2;

      swap_chain_description.Scaling     = DXGI_SCALING_STRETCH;
      swap_chain_description.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swap_chain_description.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
      swap_chain_description.Flags       = 0;

      HRESULT result = dxgi_factory->CreateSwapChainForHwnd(device, global_state->window_handle,
                                                            &swap_chain_description,
                                                            0, 0, &swap_chain);
      expect(SUCCEEDED(result));

      dxgi_factory->Release();
    }

    {
      global_state->render_context.swap_chain           = swap_chain;
      global_state->render_context.device               = device;
      global_state->render_context.device_context       = device_context;
    };

    String_Const_utf8 shader_source_path   = string_literal_init_type("..\\src\\platform_win32\\shaders.hlsl", utf8);
    Handle           *shader_source_handle = make_handle(shader_source_path, Handle_Kind_File);

    Vertex_Shader renderer_vertex_shader = {};
    Pixel_Shader  renderer_pixel_shader = {};

    ID3DBlob *vertex_shader_blob =
      (ID3DBlob *) render_load_vertex_shader(shader_source_handle, &renderer_vertex_shader, true);
    render_load_pixel_shader(shader_source_handle, &renderer_pixel_shader, true);
    safe_release(vertex_shader_blob);

    String_Const_utf8 triangle_shader_source_path =
      string_literal_init_type("..\\src\\platform_win32\\triangle_shaders.hlsl", utf8);
    Handle *triangle_shader_source_handle = make_handle(triangle_shader_source_path, Handle_Kind_File);

    String_Const_utf8 circle_shader_source_path = 
      string_literal_init_type("..\\src\\platform_win32\\circle_shaders.hlsl", utf8);
    Handle *circle_shader_source_handle = make_handle(circle_shader_source_path, Handle_Kind_File);

    Vertex_Shader triangle_vertex_shader = {};
    Pixel_Shader  triangle_pixel_shader = {};

    Vertex_Shader circle_vertex_shader = {};
    Pixel_Shader  circle_pixel_shader = {};

    ID3DBlob *triangle_vertex_shader_blob =
      (ID3DBlob *) render_load_vertex_shader(triangle_shader_source_handle, &triangle_vertex_shader, true);
    render_load_pixel_shader(triangle_shader_source_handle, &triangle_pixel_shader, true);
    safe_release(triangle_vertex_shader_blob);

    ID3DBlob *circle_vertex_shader_blob =
      (ID3DBlob *) render_load_vertex_shader(circle_shader_source_handle, &circle_vertex_shader, true);
    render_load_pixel_shader(circle_shader_source_handle, &circle_pixel_shader, true);
    safe_release(circle_vertex_shader_blob);

    ID3D11ShaderResourceView *cubemap_texture_view = NULL;
    render_create_cubemap(skybox_bitmaps, array_count(skybox_bitmaps), &cubemap_texture_view);

    UI_Context *ui = ui_get_context();

    global_state->running        = true;
    global_state->window_resized = true;

    u64 last_hpt = platform_get_high_precision_timer();
    u64 last_pts = platform_get_processor_time_stamp();

    f64 last_frame_time           = platform_convert_high_precision_time_to_seconds(last_hpt);
    u64 last_frame_time_in_cycles = 0;

    global_state->dt = 0.0;

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

    // NOTE(antonio): experimental change
    global_state->main_fiber_address = ConvertThreadToFiber(NULL);
    expect_message(global_state->main_fiber_address != NULL, "expected to use fiber path");

    void *win32_message_fiber_handle = CreateFiber(0, &win32_message_fiber, NULL);
    expect_message(global_state->main_fiber_address != NULL, "could not create a fiber for messages");

    const uintptr_t render_thread_args[] = {1};
    HANDLE render_thread_handle = CreateThread(NULL, 0, render_thread_proc, (void *) render_thread_args, 0, NULL);
    unused(render_thread_handle);

    Vertex_Buffer_Element cube_vertices[] = 
    {
#define CUBE_VERTEX(...) vbe(__VA_ARGS__)
#include "../trader_cube_vertices.h"
#undef CUBE_VERTEX
    };

    Matrix_f32_4x4 projection = matrix4x4_symmetric_projection(1.0f, 100.0f, 1.0f, 1.0f);
    Matrix_f32_4x4 view       = matrix4x4_diagonals(1.0f, 1.0f, 1.0f, 1.0f);

    V3_f32 points[] = {V3(0.0f, 0.0f, 0.0f), V3(0.0f, 0.0f, 0.5f), V3(0.0f, 1.0f, 0.5f), V3(0.5f, 0.75f, 0.5f)};
    // u32 sector_count = 4;
    // f32 point_count = (f32) array_count(points);

    Bucket_List particle_buckets = bucket_list_make(global_arena, 
                                                    mb(1),
                                                    kb(4),
                                                    sizeof(Circle_Particle_Header),
                                                    16,
                                                    scu8l("Circle Particles"));

    batch_make_circle_particles(&particle_buckets, 1.0f, 2.0f, 100, 200);

    Player_Context *player_context = player_get_context();
    unused(player_context);

    // TODO(antonio): remove this dumbass synchronization method
    volatile u8 *ring_buffer_start = common_render->command_queue.start;
    while (!ring_buffer_start)
    {
      ring_buffer_start = common_render->command_queue.start;
    }

    while (global_state->running)
    {
      TIMED_BLOCK_START();

      SwitchToFiber(win32_message_fiber_handle);
      global_state->render_thread_can_start_processing = true;

      if (global_state->window_resized && !IsIconic(global_state->window_handle))
      {
        global_state->window_resized = false;

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

        Render_Command *resize = (Render_Command *) common_render->command_queue.write;
        resize->kind = rck_resize;
        render_push_commands(1);
      }

      Render_Command *clear = (Render_Command *) common_render->command_queue.write;

      clear->kind = rck_clear;

      clear->clear.background_color = rgba_black;
      clear->clear.clear_render_target = true;
      clear->clear.clear_depth_stencil = true;

      render_push_commands(1);

      player_apply_input();

      Rect_f32 client_rect = render_get_client_rect();
      platform_collect_notifications();

      ui_initialize_frame();
      panel_float_index = 0;

      ui_push_background_color(rgba_from_u8(0, 0, 100, 255));
      ui_make_panel(axis_split_vertical, &panel_floats[panel_float_index++], string_literal_init_type("first", utf8));
      ui_do_string(string_literal_init_type("Hello World!", utf8));
      ui_do_formatted_string("Mouse: (%.0f, %.0f)", ui->mouse_pos.x, ui->mouse_pos.y);

      ui_do_formatted_string("Raw Input Mouse: (%.0f, %.0f)",
                             global_player_context.mouse_pos.x,
                             global_player_context.mouse_pos.y);

      ui_do_formatted_string("Raw Input Mouse Delta: (%.0f, %.0f)",
                             global_player_context.mouse_delta.x,
                             global_player_context.mouse_delta.y);

      ui_prepare_render_from_panels(ui_get_sentinel_panel(), client_rect);

      // u32 initial_draw_count = (u32) (global_state->render_context.render_data.used / sizeof(Instance_Buffer_Element));
      ui_flatten_draw_layers();

      /*
      Render_Position cylinder_rp = make_cylinder(&common_render->triangle_render_data,
                                                  1.0f, 1.0f, 1.0f, 16, 1);

      make_cylinder_along_path(&common_render->triangle_render_data, points, (u32) point_count, 0.05f, sector_count);
      Render_Position player_rp = make_player(&common_render->triangle_render_data);

      Bucket_List *bucket_lists[] = {&particle_buckets};
      Render_Position circle_rp =
        render_and_update_particles(&common_render->triangle_render_data, bucket_lists, array_count(bucket_lists));
        */

      // NOTE(antonio): instances

      global_state->main_thread_done_submitting = true;
      while (!global_state->render_thread_done_processing);

      swap_chain->Present(1, 0);

      global_state->main_thread_done_submitting   = false;
      global_state->render_thread_done_processing = false;

      arena_reset(&global_state->render_context.render_data);
      arena_reset(&global_state->render_context.triangle_render_data);

      meta_collate_timing_records();

      // Post-frame
      global_state->focus_event = focus_event_none;

      for (u32 file_index = 0;
           file_index < array_count(global_state->changed_files);
           ++file_index)
      {
        if (global_state->changed_files[file_index][0] != '\0')
        {
          zero_struct(global_state->changed_files);
        }
        else
        {
          break;
        }
      }

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

        global_state->dt = last_frame_time;
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
