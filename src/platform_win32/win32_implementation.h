#ifndef WIN32_IMPLEMENTATION_H
#include <malloc.h>

struct Vertex_Shader
{
  ID3D11VertexShader *shader;
};

struct Pixel_Shader
{
  ID3D11PixelShader  *shader;
};

struct Render_Context
{
  IDXGISwapChain1     *swap_chain;
  ID3D11Device        *device;
  ID3D11DeviceContext *device_context;

  Texture_Atlas       *atlas;
  Arena                render_data;
};

#pragma pack(push, 4)
struct Global_Platform_State
{
  Arena          temp_arena;

  Render_Context render_context;

  HWND           window_handle;

  HANDLE         notify_iocp;
  HANDLE         notify_dir;
  HANDLE         notify_dir_iocp;
  OVERLAPPED     notify_overlapped;

  // TODO(antonio): make part of global arena
  u8             _changed_files[kb(1)];
  utf8           changed_files[8][128];
};
#pragma pack(pop)

global Global_Platform_State win32_global_state = {};

struct Socket
{
  SOCKET socket;

  SSL   *ssl_state;
  BIO   *ssl_buffer;

  //u8     websocket_key[32];
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

internal Global_Platform_State *get_global_platform_state()
{
  Global_Platform_State *state = &win32_global_state;
  return(state);
}

internal Rect_f32 render_get_client_rect()
{
  Rect_f32 client_rect = {};

  RECT win32_client_rect = {};
  GetClientRect(get_global_platform_state()->window_handle, &win32_client_rect);

  client_rect.x0 = 0;
  client_rect.y0 = 0;

  client_rect.x1 = (f32) (win32_client_rect.right  - win32_client_rect.left);
  client_rect.y1 = (f32) (win32_client_rect.bottom - win32_client_rect.top);

  return(client_rect);
}

/*
internal Asset_Handle render_make_texture(Render_Context *context, void *texture_data, u64 width, u64 height, u64 channels)
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
*/

internal b32 platform_open_file(utf8 *file_path, u64 file_path_size, Handle *out_handle)
{
  b32 result = false;

  utf8 *file_path_copy = (utf8 *) arena_push_zero(&win32_global_state.temp_arena, file_path_size);
  if (file_path_copy != NULL)
  {
    copy_memory_block(file_path_copy, file_path, file_path_size);

    utf16 file_path_utf16[512] = {};
    u32 bytes_written =
      (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                (LPCCH) file_path, (int) file_path_size,
                                (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    assert(bytes_written == file_path_size);
    HANDLE file_handle = CreateFileW((LPCWSTR) file_path_utf16,
                                     GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL, OPEN_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle != INVALID_HANDLE_VALUE)
    {
      out_handle->file_handle = file_handle;
      result = true;
    }
  }

  return(result);
}

internal File_Buffer platform_read_entire_file(Handle *handle)
{
  File_Buffer file_buffer = {};

  assert((handle != NULL) && "idiot! you have to provide a good handle");

  LARGE_INTEGER large_file_size;
  if (GetFileSizeEx(handle->file_handle, &large_file_size))
  {
    u64 file_size = large_file_size.QuadPart;
    unused(file_size);

    u8 *file_buffer_data = (u8 *) temp_arena_push(&win32_global_state.temp_arena, file_size);
    if (file_buffer_data)
    {
      SetFilePointer(handle->file_handle, 0, NULL, FILE_BEGIN);

      u32 bytes_read = 0;
      if (ReadFile(handle->file_handle, file_buffer_data, (u32) file_size, (LPDWORD) &bytes_read, NULL))
      {
        assert((bytes_read == file_size) && "Win32 Error: bytes read does not match expected");

        file_buffer.data           = file_buffer_data;
        file_buffer.size           = file_size;
      }
      else
      {
        assert(!"Win32 Error: could not read file");
      }
    }
  }
  else
  {
    assert(!"could not get file size");
  }

  return(file_buffer);
}

internal File_Buffer platform_open_and_read_entire_file(Arena *arena, utf8 *file_path, u64 file_path_size)
{
  File_Buffer file_buffer = {};

  utf16 file_path_utf16[512] = {};
  u32 bytes_written =
    (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
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

  return(file_buffer);
}

internal void platform_push_notify_dir(utf8 *dir_path, u64 dir_path_size)
{
  if ((dir_path != NULL) && (win32_global_state.notify_iocp != INVALID_HANDLE_VALUE))
  {
    utf16 file_path_utf16[512] = {};
    u32 bytes_written = (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                                  (LPCCH) dir_path, (int) dir_path_size,
                                                  (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    if (bytes_written == dir_path_size)
    {
      HANDLE dir_handle = CreateFileW((LPCWSTR) file_path_utf16,
                                       GENERIC_READ, FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                       NULL);
      if (dir_handle != INVALID_HANDLE_VALUE)
      {
        // TODO(antonio): completion key?
        HANDLE notify_dir_iocp = CreateIoCompletionPort(dir_handle, win32_global_state.notify_iocp, 0, 0);
        if (notify_dir_iocp != INVALID_HANDLE_VALUE)
        {
          win32_global_state.notify_dir      = dir_handle;
          win32_global_state.notify_dir_iocp = notify_dir_iocp;
        }
        else
        {
        }
      }
      else
      {
        // TODO(antonio): logging
      }
    }
    else
    {
      // TODO(antonio): loggindffag
    }
  }
  else
  {
    // TODO(antonio): logging
  }
}

internal void platform_start_collect_notifications(void)
{
  win32_global_state.notify_overlapped = {};

  FILE_NOTIFY_INFORMATION *changes = (FILE_NOTIFY_INFORMATION *) win32_global_state._changed_files;
  assert(((ptr_val) changes & 0x3) == 0);

  DWORD notify_filter = FILE_NOTIFY_CHANGE_LAST_WRITE  | FILE_NOTIFY_CHANGE_CREATION   |
                        FILE_NOTIFY_CHANGE_SIZE        | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                        FILE_NOTIFY_CHANGE_LAST_ACCESS;

  u32 bytes_written = 0;
  BOOL got_changes =
    ReadDirectoryChangesW(win32_global_state.notify_dir,
                          (void *) changes,
                          (DWORD) sizeof(win32_global_state._changed_files),
                          TRUE,
                          notify_filter,
                          (DWORD *) &bytes_written,
                          &win32_global_state.notify_overlapped,
                          NULL);
  if (!got_changes)
  {
    DWORD error; error = GetLastError();
    assert(!"expected ReadDirectoryChangesW to succeed");
  }
}

internal void platform_collect_notifications(void)
{
  if (win32_global_state.notify_dir != INVALID_HANDLE_VALUE)
  {
    OVERLAPPED *overlapped        = NULL;
    void       *completion_key    = NULL;
    u32         bytes_transferred = 0;

    BOOL completion_status =
      GetQueuedCompletionStatus(win32_global_state.notify_iocp,
                                (DWORD *) &bytes_transferred,
                                (PULONG_PTR) &completion_key,
                                &overlapped,
                                0);
    if (completion_status)
    {
      zero_struct(win32_global_state.changed_files);

      i32 cur_index = 0;
      FILE_NOTIFY_INFORMATION *cur = (FILE_NOTIFY_INFORMATION *) win32_global_state._changed_files;

      do
      {
        utf16 last_char = cur->FileName[cur->FileNameLength];
        cur->FileName[cur->FileNameLength] = 0;

        WideCharToMultiByte(CP_UTF8, 0,
                            cur->FileName,
                            (i32)   c_string_count(cur->FileName),
                            (LPSTR) win32_global_state.changed_files[cur_index],
                            array_count(win32_global_state.changed_files[cur_index]),
                            NULL, NULL);

        cur->FileName[cur->FileNameLength] = last_char;

        cur_index++;
        cur = (FILE_NOTIFY_INFORMATION *) (((u8 *) cur) + cur->NextEntryOffset);
      } while (cur->NextEntryOffset != 0);

      zero_memory_block(win32_global_state._changed_files,
                        min(bytes_transferred, sizeof(win32_global_state._changed_files)));

      platform_start_collect_notifications();
    }
    else 
    {
      // TODO(antonio): log no change?
      DWORD error;
      error = GetLastError();
    }
  }
}

internal b32 platform_did_file_change(utf8 *file_name, u64 file_name_length)
{
  b32 file_changed = false;

  for (u64 changed_file_index = 0;
       changed_file_index < array_count(win32_global_state.changed_files);
       ++changed_file_index)
  {
    utf8 *cur_changed_file = win32_global_state.changed_files[changed_file_index];

    if (cur_changed_file[0] != 0)
    {
      u64 min_length = min(file_name_length, array_count(win32_global_state.changed_files[0]));
      if (!compare_memory_block(file_name, cur_changed_file, min_length))
      {
        file_changed = true;
        break;
      }
    }
    else
    {
      break;
    }
  }

  return(file_changed);
}

global_const u8 platform_path_separator = '\\';
global_const u8 unix_path_separator = '/';

internal String_Const_utf8 platform_get_file_name_from_path(String_Const_utf8 *path)
{
  String_Const_utf8 result = {};

  u64 last_separator_pos = 0;
  for (u64 path_char_index = 0;
       path_char_index < path->size;
       ++path_char_index)
  {
    if ((path->str[path_char_index] == platform_path_separator) ||
        (path->str[path_char_index] == unix_path_separator))
    {
      last_separator_pos = path_char_index;
    }
  }

  u64 char_pos = (last_separator_pos == 0) ? last_separator_pos : (last_separator_pos + 1);
  result.str   = &path->str[char_pos];
  result.size  = path->size - char_pos;

  return(result);
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
    i32 ssl_result = SSL_CTX_set_default_verify_paths(out_state->ssl_context);
    if (ssl_result != SSL_OK)
    {
      result = network_error_client_certificate_needed;
      network_print_error();
    }
  }

  return(result);
}

internal Network_Return_Code network_connect(Network_State *state, Socket *out_socket, String_Const_utf8 host_name, u16 port)
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

  // TODO(antonio): WSA_FLAG_OVERLAPPED
  out_socket->socket = WSASocket(addr_found->ai_family,
                                 addr_found->ai_socktype,
                                 addr_found->ai_protocol,
                                 NULL,
                                 0, 0); 

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

  i32 ssl_result;
  {
    out_socket->ssl_state = SSL_new(state->ssl_context);

    ssl_result = SSL_set_fd(out_socket->ssl_state, (int) out_socket->socket);
    if (ssl_result != SSL_OK)
    {
      network_print_error();
    }

    SSL_set_connect_state(out_socket->ssl_state);

    assert((host_name.size <= SSL_MAX_HOST_NAME_LENGTH) && "the host name is too long");
    ssl_result = SSL_set_tlsext_host_name(out_socket->ssl_state, (char *) host_name.str);
    assert((ssl_result == 1) && "no tls extension :(");

    ssl_result = SSL_add1_host(out_socket->ssl_state, (char *) host_name.str);
    assert((ssl_result == 1) && "could not add host name");
  }

  b32 connected = WSAConnectByNameA(out_socket->socket,
                                   (char *) host_name.str, (char *) port_name,
                                   0, NULL, 0, NULL, NULL, NULL);
  assert(connected && "expected connection");

  // TODO(antonio): wrap up? resumption?
  ssl_result = SSL_do_handshake(out_socket->ssl_state);
  if (ssl_result == SSL_ERROR)
  {
    network_print_error();
  }

  return(result);
}

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *to_send)
{
  Network_Return_Code result = network_ok;

  unused(state);
  assert(to_send->data != NULL);

  if (!is_nil(in_socket))
  {
    i32 bytes_sent = SSL_write(in_socket->ssl_state, to_send->data, (i32) to_send->used);
    if (bytes_sent <= 0) {
      result = network_error_send_failure;
      network_print_error();
    }
  }

  return(result);
}

internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive_buffer)
{
  unused(state);
  unused(receive_buffer);

  Network_Return_Code result = network_ok;

  assert(in_socket          != NULL);
  assert(receive_buffer != NULL);

  if (!is_nil(in_socket))
  {
    do
    {
      i32 bytes_received = SSL_read(in_socket->ssl_state, receive_buffer->data, (i32) receive_buffer->size - 1);
      if (bytes_received > 0)
      {
        receive_buffer->used = bytes_received;
        receive_buffer->data[receive_buffer->used] = 0;

        OutputDebugStringA((char * ) receive_buffer->data);
      }
      else
      {
        // TODO(antonio): why did it stop - can be success or failure
        break;
      }
    } while (1);
  }

  network_print_error();

  return(result);
}

internal Network_Return_Code network_do_websocket_handshake(Network_State *state,
                                                            Socket *in_out_socket,
                                                            String_Const_utf8 host_name,
                                                            String_Const_utf8 query_path,
                                                            String_Const_utf8 api_key)
{
  Network_Return_Code result = network_ok;

  unused(api_key);

  u8 sec_websocket_key[16] = {};
  rng_fill_buffer(sec_websocket_key, array_count(sec_websocket_key));

  u8 websocket_key[32] = {};
  base64_encode(websocket_key, sec_websocket_key, array_count(sec_websocket_key));

#if 0
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Version: 13
Origin: http://example.com
#endif

  u8 websocket_handshake[1024] = {};
  Buffer websocket_handshake_send = buffer_from_fixed_size(websocket_handshake);

  websocket_handshake_send.used =
    stbsp_snprintf((char *) websocket_handshake, array_count(websocket_handshake),
                   "GET /%s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Upgrade: websocket\r\n"
                   "Connection: Upgrade\r\n"
                   "Sec-WebSocket-Key: %s\r\n"
                   "Sec-WebSocket-Protocol: chat, superchat\r\n"
                   "Sec-WebSocket-Version: 13\r\n",
                   (char *) query_path.str,
                   (char *) host_name.str,
                   (char *) websocket_key);

  network_send_simple(state, in_out_socket, &websocket_handshake_send);

  u8 _websocket_receive[kb(4)] = {};
  Buffer websocket_receive = buffer_from_fixed_size(_websocket_receive);

  network_receive_simple(state, in_out_socket, &websocket_receive);

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

internal void platform_print(const char *format, ...)
{
  OutputDebugStringA(format);
}

internal Arena arena_alloc(u64 size, u64 alignment, void *start)
{
  Arena arena = {};

  u8 *allocated = (u8 *) VirtualAlloc(start, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  assert((allocated != NULL) && "virtual alloc failed");

  arena.start     = allocated;
  arena.size      = size;
  arena.used      = 0;
  arena.alignment = alignment;

  return(arena);
}


internal void *render_load_vertex_shader(Handle *shader_handle, Vertex_Shader *shader, b32 force)
{
  void *blob = NULL;

  u64 file_name_length = c_string_length(shader_handle->id, array_count(shader_handle->id));
  if (force || platform_did_file_change(shader_handle->id, file_name_length))
  {
    safe_release(shader->shader);

    // TODO(antonio): will have to read file twice
    File_Buffer temp_shader_source = platform_read_entire_file(shader_handle);
    {
      ID3DBlob *vertex_shader_blob = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      HRESULT result = D3DCompile(temp_shader_source.data, temp_shader_source.size, NULL, NULL, NULL,
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
        }

        MessageBoxA(0, error_string.str, "Shader Compiler Error", MB_ICONERROR | MB_OK);
      }

      ID3D11Device *device = win32_global_state.render_context.device;
      result = device->CreateVertexShader(vertex_shader_blob->GetBufferPointer(),
                                          vertex_shader_blob->GetBufferSize(),
                                          NULL, &shader->shader);

      assert(SUCCEEDED(result));
      safe_release(shader_compile_errors_blob);

      blob = (void *) vertex_shader_blob;
    }
  }

  return(blob);
}

internal void render_load_pixel_shader(Handle *shader_handle, Pixel_Shader *shader, b32 force)
{
  u64 file_name_length = c_string_length(shader_handle->id, array_count(shader_handle->id));
  if (force || platform_did_file_change(shader_handle->id, file_name_length))
  {
    safe_release(shader->shader);

    // TODO(antonio): will have to read file twice
    File_Buffer temp_shader_source = platform_read_entire_file(shader_handle);
    {
      ID3DBlob *pixel_shader_blob          = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      HRESULT return_code = D3DCompile(temp_shader_source.data, temp_shader_source.size, NULL, NULL, NULL,
                                       "PS_Main", "ps_5_0", 0, 0, &pixel_shader_blob, &shader_compile_errors_blob);

      if (FAILED(return_code))
      {
        String_Const_char error_string = {};

        if (shader_compile_errors_blob)
        {
          error_string =
          {
            (char *) shader_compile_errors_blob->GetBufferPointer(),
            shader_compile_errors_blob->GetBufferSize()
          };
        }

        MessageBoxA(0, error_string.str, "Shader Compiler Error", MB_ICONERROR | MB_OK);
      }

      ID3D11Device *device = win32_global_state.render_context.device;
      return_code = device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(),
                                              pixel_shader_blob->GetBufferSize(),
                                              NULL,
                                              &shader->shader);

      assert(SUCCEEDED(return_code));

      safe_release(shader_compile_errors_blob);
      pixel_shader_blob->Release();
    }
  }
}

internal void render_draw_text(utf8 *text, u64 text_size, f32 *baseline_x, f32 *baseline_y)
{
  Texture_Atlas *atlas = win32_global_state.render_context.atlas;
  Instance_Buffer_Element *render_elements =
    push_array(&win32_global_state.render_context.render_data, Instance_Buffer_Element, text_size);

  f32 font_scale = stbtt_ScaleForPixelHeight(&atlas->font_info, atlas->heights[0]);

  f32 cur_x = *baseline_x;
  f32 cur_y = *baseline_y;

  for (u64 text_index = 0;
       text_index < text_size;
       ++text_index)
  {
    Instance_Buffer_Element *cur_element     = render_elements  +  text_index;
    stbtt_packedchar        *cur_packed_char = atlas->char_data + (text[text_index] - starting_code_point);

    cur_element->pos = 
    {
      cur_x + cur_packed_char->xoff,
      cur_y + cur_packed_char->yoff,
      1.0f
    };

    cur_element->size = 
    {
      0.0f,
      0.0f,
      (f32) (cur_packed_char->xoff2 - cur_packed_char->xoff),
      (f32) (cur_packed_char->yoff2 - cur_packed_char->yoff)
    };

    cur_element->uv = 
    {
      (f32) cur_packed_char->x0,
      (f32) cur_packed_char->y0,
      (f32) cur_packed_char->x1,
      (f32) cur_packed_char->y1,
    };

    cur_element->color = {1.0f, 1.0f, 1.0f, 1.0f};

    f32 kern_advance = 0.0f;
    if (text_index < (text_size - 1))
    {
      kern_advance = font_scale *
                     stbtt_GetCodepointKernAdvance(&atlas->font_info,
                                                   text[text_index],
                                                   text[text_index + 1]);
    }

    cur_x += kern_advance + cur_packed_char->xadvance;
  }

  *baseline_x = cur_x;
}

#define WIN32_IMPLEMENTATION_H
#endif
