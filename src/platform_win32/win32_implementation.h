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
};

struct Socket
{
  SOCKET socket;

  SSL   *ssl_state;

  BIO   *ssl_send;
  BIO   *ssl_receive;
};
Socket nil_socket = {INVALID_SOCKET};

internal b32 is_nil(Socket *check)
{
  b32 result = (check->socket == nil_socket.socket);
  return(result);
}

internal void make_nil(Socket *s)
{
  s->socket = INVALID_SOCKET;
}

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

internal Network_Return_Code network_startup(Network_State *out_state)
{
  Network_Return_Code result = network_ok;

  WSADATA winsock_metadata = {};
  i32 winsock_result = WSAStartup(MAKEWORD(2, 2), &winsock_metadata);
  assert((winsock_result == 0) && "expected winsock dll to load");

  {
    assert((out_state != NULL) && "expected out_state to be valid");

    const SSL_METHOD *client_method = NULL;
    client_method = TLS_client_method();

    out_state->ssl_context = SSL_CTX_new(client_method);
    assert((out_state->ssl_context != NULL) && "expected to create an ssl context");

    SSL_CTX_set_verify(out_state->ssl_context, SSL_VERIFY_PEER, NULL);

    // TODO(antonio): certificate?
  }

  return(result);
}

Network_Return_Code network_connect(Network_State *state, String_Const_utf8 host_name, u16 port, Socket *out_socket)
{
  unused(state);

  Network_Return_Code result = network_ok;

  assert(out_socket    != NULL);
  assert(host_name.str != NULL);

  make_nil(out_socket);

  u8 port_name[64] = {};
  stbsp_snprintf((char *) port_name, sizeof(port_name), "%u", port);

  addrinfo *addr_found;
  addrinfo  addr_hints = {};
  {
    addr_hints.ai_flags    = AI_PASSIVE;
    addr_hints.ai_family   = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
  }

  i32 found_addr = getaddrinfo((char *) host_name.str, (char *) port_name, &addr_hints, &addr_found);
  assert((found_addr == 0) && (addr_found != NULL) && "could not get address info for the given host name");

  out_socket->socket = WSASocket(addr_found->ai_family,
                                 addr_found->ai_socktype,
                                 addr_found->ai_protocol,
                                 NULL,
                                 0,
                                 WSA_FLAG_OVERLAPPED); 

  assert((out_socket->socket != INVALID_SOCKET) && "expected connection");

  // TODO(antonio): investigate how the following may change the performance of the socket
#if 0
  BOOL send_no_buffering = 0;
  i32 set_result = setsockopt(out_socket->socket,
                              SOL_SOCKET,
                              SO_SNDBUF,
                              (char *) &send_no_buffering,
                              sizeof(send_no_buffering));
  assert((set_result == 0) && "could not disable send buffering");
#endif

  b32 connected = WSAConnectByNameA(out_socket->socket,
                                   (char *) host_name.str, (char *) port_name,
                                   0, NULL, 0, NULL, NULL, NULL);
  assert(connected && "expected connection");

  out_socket->ssl_state = SSL_new(state->ssl_context);

  // TODO(antonio): use our memory buffers
  out_socket->ssl_send    = BIO_new(BIO_s_mem());
  out_socket->ssl_receive = BIO_new(BIO_s_mem());

  BIO_set_mem_eof_return(out_socket->ssl_send, -1);
  BIO_set_mem_eof_return(out_socket->ssl_receive, -1);

  SSL_set_bio(out_socket->ssl_state, out_socket->ssl_receive, out_socket->ssl_send);
  SSL_set_connect_state(out_socket->ssl_state);

  return(result);
}

Network_Return_Code network_send(Network_State *state, Socket *in_socket, Buffer to_send)
{
  unused(state);
  unused(to_send);

  Network_Return_Code result = network_ok;

  assert(to_send.data != NULL);

  u64 send_size = (u64) to_send.used;
  unused(send_size);

  if (!is_nil(in_socket))
  {
  }

  return(result);
}

internal Network_Return_Code network_receive(Network_State *state, Socket *in_socket, Buffer *out_receive_buffer)
{
  unused(state);
  unused(out_receive_buffer);

  Network_Return_Code result = network_ok;

  assert(in_socket          != NULL);
  assert(out_receive_buffer != NULL);

  if (!is_nil(in_socket))
  {
  }

  return(result);
}

internal Network_Return_Code network_disconnect(Network_State *state, Socket *in_out_socket)
{
  unused(state);

  Network_Return_Code result = network_ok;

  assert(in_out_socket != NULL);
  shutdown(in_out_socket->socket, SD_BOTH);
  closesocket(in_out_socket->socket);

  make_nil(in_out_socket);

  return(result);
}

internal Network_Return_Code network_cleanup(Network_State *state)
{
  unused(state);

  Network_Return_Code result = network_ok;

  WSACleanup();

  return(result);
}

internal DWORD iocp_thread_proc(LPVOID _iocp_handle)
{
  DWORD result = 0;

  HANDLE iocp_handle = (HANDLE) _iocp_handle;
  unused(iocp_handle);

  return(result);
}

#define WIN32_IMPLEMENTATION_H
#endif
