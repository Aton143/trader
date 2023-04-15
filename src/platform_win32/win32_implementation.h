#ifndef WIN32_IMPLEMENTATION_H
#include <malloc.h>

struct Global_Platform_State
{
  HWND      window_handle;
};

global Global_Platform_State win32_global_state = {};

struct Render_Context
{
  IDXGISwapChain1     *swap_chain;
  ID3D11Device        *device;
  ID3D11DeviceContext *device_context;

  f32                  render_width;
  f32                  render_height;
};

Global_Platform_State *get_global_platform_state()
{
  Global_Platform_State *state = &win32_global_state;
  return(state);
}

Rect_f32 render_get_client_rect()
{
  Rect_f32 client_rect = {};

  RECT win32_client_rect = {};
  GetClientRect(get_global_platform_state()->window_handle, &win32_client_rect);

  client_rect.x0 = 0;
  client_rect.y0 = 0;

  client_rect.x1 = (f32) (win32_client_rect.right  - win32_client_rect.left);
  client_rect.x1 = (f32) (win32_client_rect.bottom - win32_client_rect.top);

  return(client_rect);
}

void render_put_context(Render_Context *context)
{
  Rect_f32 client_rect = render_get_client_rect();

  context->render_width  = client_rect.x1;
  context->render_height = client_rect.y1;
}

Asset_Handle render_make_texture(Render_Context *context, void *texture_data, u64 width, u64 height, u64 channels)
{
  Asset_Handle handle = nil_handle;

  const DXGI_FORMAT formats[5] =
  {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM};

  assert(is_between_inclusive(1, channels, 4) && (channels != 3));

  ID3D11ShaderResourceView *texture_view = NULL;
  {
    D3D11_TEXTURE2D_DESC texture_description = {};

    texture_description.Width            = (UINT) height;
    texture_description.Height           = (UINT) width;
    texture_description.MipLevels        = 1;
    texture_description.ArraySize        = 1;
    texture_description.Format           = formats[channels];
    texture_description.SampleDesc.Count = 1;
    texture_description.Usage            = D3D11_USAGE_DEFAULT;
    texture_description.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    texture_description.CPUAccessFlags   = 0;

    D3D11_SUBRESOURCE_DATA subresource = {};

    subresource.pSysMem          = texture_data;
    subresource.SysMemPitch      = (UINT) (texture_description.Width * channels);
    subresource.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture_2d = NULL;
    HRESULT result = context->device->CreateTexture2D(&texture_description, &subresource, &texture_2d);

    assert(SUCCEEDED(result));

    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = {};

    shader_resource_view_description.Format                    = formats[channels];
    shader_resource_view_description.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_resource_view_description.Texture2D.MipLevels       = texture_description.MipLevels;
    shader_resource_view_description.Texture2D.MostDetailedMip = 0;

    result = context->device->CreateShaderResourceView(texture_2d, &shader_resource_view_description, &texture_view);
    texture_2d->Release();

    assert(SUCCEEDED(result));
  }

  return(handle);
}

File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size)
{
  File_Buffer file_buffer = {};

  utf8 *file_path_copy = (utf8 *) arena_push_zero(arena, file_path_size);
  if (file_path_copy != NULL)
  {
    copy_memory_block(file_path_copy, file_path, file_path_size);

    utf16 file_path_utf16[512] = {};

    u32 bytes_written = (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                                  (LPCCH) file_path, (int) file_path_size,
                                                  (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    if (bytes_written == file_path_size)
    {
      HANDLE file_handle = CreateFileW((LPCWSTR) file_path_utf16,
                                       GENERIC_READ, FILE_SHARE_READ,
                                       NULL, OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

      if (file_handle != INVALID_HANDLE_VALUE)
      {
        LARGE_INTEGER large_file_size;
        if (GetFileSizeEx(file_handle, &large_file_size))
        {
          u64 file_size = large_file_size.QuadPart;
          u8 *file_buffer_data = (u8 *) arena_push(arena, file_size);

          if (file_buffer_data)
          {
            u32 bytes_read = 0;
            if (ReadFile(file_handle, file_buffer_data, (u32) file_size, (LPDWORD) &bytes_read, NULL))
            {
              assert((bytes_read == file_size) || !"Win32 Error: bytes read does not match expected");

              file_buffer.data           = file_buffer_data;
              file_buffer.file_path.str  = file_path_copy;
              file_buffer.file_path.size = file_path_size;
              file_buffer.size           = file_size;

              CloseHandle(file_handle);
            }
            else
            {
              assert(!"Win32 Error: could not read file");
            }
          }
          else
          {
            assert(!"expected arena allocation to succeed");
          }
        }
        else
        {
          assert(!"Win32 Error: could not get file size");
        }
      }
      else
      {
        assert(!"Win32 Error: could not open file");
      }
    }
    else
    {
      assert(!"Win32 Error: could not convert the file path to UTF-16");
    }
  }
  else
  {
    assert(!"expected arena allocation to succeed");
  }

  return(file_buffer);
}

#define WIN32_IMPLEMENTATION_H
#endif
