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
  Arena                triangle_render_data;
};

typedef u64 Focus_Event;
enum
{
  focus_event_none,

  focus_event_gain,
  focus_event_lose,

  focus_event_count
};

#pragma pack(push, 4)
struct Global_Platform_State
{
  Temp_Arena     temp_arena;

  Render_Context render_context;
  UI_Context     ui_context;

  Focus_Event    focus_event;
  f64            dt;

  HWND           window_handle;

  HANDLE         notify_iocp;
  HANDLE         notify_dir;
  HANDLE         notify_dir_iocp;
  OVERLAPPED     notify_overlapped;

#if !SHIP_MODE
  u64 frame_count;
#endif

  // TODO(antonio): make part of global arena
  u8             _changed_files[kb(1)];
  utf8           changed_files[8][128];
};
#pragma pack(pop)

global Global_Platform_State win32_global_state = {};

internal UI_Context *ui_get_context(void)
{
  UI_Context *context = &win32_global_state.ui_context;
  return(context);
}

internal Render_Context *render_get_context(void)
{
  Render_Context *context = &win32_global_state.render_context;
  return(context);
}

internal Arena *get_temp_arena(void)
{
  Arena *temp_arena = &win32_global_state.temp_arena.arena;

  if (win32_global_state.temp_arena.wait > 0)
  {
    win32_global_state.temp_arena.wait--;
  }
  else
  {
    temp_arena->used = 0;
  }

  return(temp_arena);
}

internal void set_temp_arena_wait(u64 wait)
{
  win32_global_state.temp_arena.wait = wait;
}

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

internal Global_Platform_State *platform_get_global_state(void)
{
  Global_Platform_State *state = &win32_global_state;
  return(state);
}

internal Rect_f32 render_get_client_rect(void)
{
  Rect_f32 client_rect = {};

  RECT win32_client_rect = {};
  GetClientRect(platform_get_global_state()->window_handle, &win32_client_rect);

  client_rect.x0 = 0;
  client_rect.y0 = 0;

  client_rect.x1 = (f32) (win32_client_rect.right  - win32_client_rect.left);
  client_rect.y1 = (f32) (win32_client_rect.bottom - win32_client_rect.top);

  return(client_rect);
}

internal void meta_init(void)
{
  LARGE_INTEGER high_precision_timer_frequency = {};
  QueryPerformanceFrequency(&high_precision_timer_frequency);

  meta_info.high_precision_timer_frequency = high_precision_timer_frequency.QuadPart; 

  Arena *temp_arena = get_temp_arena();
  set_temp_arena_wait(1);

  temp_arena->used = copy_string_lit(temp_arena->start, ".\\logs\\");
  temp_arena->used -= 1;

  temp_arena->used +=
    GetDateFormatA(LOCALE_NAME_USER_DEFAULT,
                    0,
                    NULL,
                    "yyyy'_'MM'_'dd'_'",
                    (char *) &temp_arena->start[temp_arena->used],
                    (int) (temp_arena->size - temp_arena->used - 1));
  temp_arena->used -= 1;

  temp_arena->used += 
    GetTimeFormatA(LOCALE_NAME_USER_DEFAULT,
                    0,
                    NULL,
                    "HH'_'mm'_'ss",
                    (char *) &temp_arena->start[temp_arena->used],
                    (int) (temp_arena->size - temp_arena->used - 1));
  temp_arena->used -= 1;

  temp_arena->used += copy_string_lit(&temp_arena->start[temp_arena->used], ".log");
  temp_arena->used -= 1;

  platform_open_file_for_appending(temp_arena->start, temp_arena->used, &meta_info.log_handle);
}

internal void meta_log(utf8 *format, ...)
{
  va_list arguments;
  va_start(arguments, format);
  platform_append_to_file(&meta_info.log_handle, format, arguments);
  va_end(arguments);
}

internal b32 platform_open_file(utf8 *file_path, u64 file_path_size, Handle *out_handle)
{
  b32 result = false;

  Arena *temp_arena = get_temp_arena();

  utf8 *file_path_copy = (utf8 *) arena_push_zero(temp_arena, file_path_size);
  if (file_path_copy != NULL)
  {
    copy_memory_block(file_path_copy, file_path, file_path_size);

    utf16 file_path_utf16[512] = {};
    u32 bytes_written =
      (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                (LPCCH) file_path, (int) file_path_size,
                                (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    expect(bytes_written == file_path_size);
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

internal b32 platform_open_file_for_appending(utf8 *file_path, u64 file_path_size, Handle *out_handle)
{
  b32 result = false;

  Arena *temp_arena = get_temp_arena();
  utf8 *file_path_copy = (utf8 *) arena_push_zero(temp_arena, file_path_size);
  if (file_path_copy != NULL)
  {
    copy_memory_block(file_path_copy, file_path, file_path_size);

    utf16 file_path_utf16[512] = {};
    u32 bytes_written =
      (u32) MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,
                                (LPCCH) file_path, (int) file_path_size,
                                (wchar_t *) file_path_utf16, array_count(file_path_utf16));

    expect(bytes_written == file_path_size);
    HANDLE file_handle = CreateFileW((LPCWSTR) file_path_utf16,
                                     GENERIC_WRITE | FILE_APPEND_DATA, FILE_SHARE_READ,
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
  Arena *temp_arena = get_temp_arena();

  expect_message(handle != NULL, "idiot! you have to provide a good handle");

  LARGE_INTEGER large_file_size;
  if (GetFileSizeEx(handle->file_handle, &large_file_size))
  {
    u64 file_size = large_file_size.QuadPart;
    unused(file_size);

    u8 *file_buffer_data = (u8 *) arena_push(temp_arena, file_size);
    if (file_buffer_data)
    {
      SetFilePointer(handle->file_handle, 0, NULL, FILE_BEGIN);

      u32 bytes_read = 0;
      if (ReadFile(handle->file_handle, file_buffer_data, (u32) file_size, (LPDWORD) &bytes_read, NULL))
      {
        expect_message(bytes_read == file_size, "Win32 Error: bytes read does not match expected");

        file_buffer.data           = file_buffer_data;
        file_buffer.size           = file_size;
      }
      else
      {
        expect_message(false, "Win32 Error: could not read file");
      }
    }
  }
  else
  {
    expect_message(false, "could not get file size");
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
            expect_message(bytes_read == file_size, "Win32 Error: bytes read does not match expected");

            file_buffer.data           = file_buffer_data;
            file_buffer.size           = file_size;

            CloseHandle(file_handle);
          }
          else
          {
            expect_message(false, "Win32 Error: could not read file");
          }
        }
        else
        {
          expect_message(false, "expected arena allocation to succeed");
        }
      }
      else
      {
        expect_message(false, "Win32 Error: could not get file size");
      }
    }
    else
    {
      expect_message(false, "Win32 Error: could not open file");
    }
  }
  else
  {
    expect_message(false, "Win32 Error: could not convert the file path to UTF-16");
  }

  return(file_buffer);
}

internal b32 platform_append_to_file(Handle *handle, utf8 *format, va_list args)
{
  b32 result = false;
  expect_message(handle != NULL, "expected a file handle, numbnuts");

  Arena *temp_arena = get_temp_arena();
  String_utf8 sprinted_text =
  {
    (utf8 *) arena_push(temp_arena, temp_arena->size),
    0,
    temp_arena->size
  };

  sprinted_text.size = stbsp_vsnprintf((char *) sprinted_text.str, (i32) sprinted_text.cap - 1, (char *) format, args);

  u32 bytes_written = 0;
  WriteFile(handle->file_handle, sprinted_text.str, (DWORD) sprinted_text.size, (DWORD *) &bytes_written, NULL);

  if (bytes_written == sprinted_text.size)
  {
    result = true;
  }

  return(result);
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
  expect(((ptr_val) changes & 0x3) == 0);

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
    expect_message(false, "expected ReadDirectoryChangesW to succeed");
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

        WideCharToMultiByte(CP_UTF8,
                            0,
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
  expect_message(winsock_result == 0, "expected winsock dll to load");

  {
    expect_message(out_state != NULL, "expected out_state to be valid");

    const SSL_METHOD *client_method = NULL;
    client_method = TLS_client_method();

    out_state->ssl_context = SSL_CTX_new(client_method);
    expect_message(out_state->ssl_context != NULL, "expected to create an ssl context");

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

  expect(out_socket    != NULL);
  expect(host_name.str != NULL);

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
  expect_message((found_addr == 0) && (addr_found != NULL), "could not get address info for the given host name");

  // TODO(antonio): WSA_FLAG_OVERLAPPED
  out_socket->socket = WSASocket(addr_found->ai_family,
                                 addr_found->ai_socktype,
                                 addr_found->ai_protocol,
                                 NULL,
                                 0, 0); 

  expect_message(out_socket->socket != INVALID_SOCKET, "expected connection");

  // TODO(antonio): investigate how the following may change the performance of the socket
#if 0
  BOOL send_no_buffering = 0;
  i32 set_result = setsockopt(out_socket->socket,
                              SOL_SOCKET,
                              SO_SNDBUF,
                              (char *) &send_no_buffering,
                              sizeof(send_no_buffering));
  expect_message(set_result == 0, "could not disable send buffering");
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

    expect_message(host_name.size <= SSL_MAX_HOST_NAME_LENGTH, "the host name is too long");
    ssl_result = SSL_set_tlsext_host_name(out_socket->ssl_state, (char *) host_name.str);
    expect_message(ssl_result == 1, "no tls extension :(");

    ssl_result = SSL_add1_host(out_socket->ssl_state, (char *) host_name.str);
    expect_message(ssl_result == 1, "could not add host name");
  }

  b32 connected = WSAConnectByNameA(out_socket->socket,
                                   (char *) host_name.str, (char *) port_name,
                                   0, NULL, 0, NULL, NULL, NULL);
  expect_message(connected, "expected connection");

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
  expect(to_send->data != NULL);

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

  expect(in_socket          != NULL);
  expect(receive_buffer != NULL);

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

  expect(in_out_socket != NULL);
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

internal void platform_debug_print(char *text)
{
  OutputDebugStringA(text);
}

internal Arena arena_alloc(u64 size, u64 alignment, void *start)
{
  Arena arena = {};

  u8 *allocated = (u8 *) VirtualAlloc(start, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  expect_message(allocated != NULL, "virtual alloc failed");

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

      expect(SUCCEEDED(result));
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

      expect(SUCCEEDED(return_code));

      safe_release(shader_compile_errors_blob);
      pixel_shader_blob->Release();
    }
  }
}

internal i64 render_get_font_height_index(f32 font_height)
{
  Texture_Atlas *atlas = win32_global_state.render_context.atlas;

  i64 result = -1;
  for (u64 font_height_index = 0;
       font_height_index < array_count(atlas->heights);
       ++font_height_index)
  {
    f32 cur_height = atlas->heights[font_height_index];
    if (approx_equal(cur_height, font_height))
    {
      result = font_height_index;
    }
  }

  return(result);
}

internal i64 render_get_packed_char_start(f32 font_height)
{
  Texture_Atlas *atlas = win32_global_state.render_context.atlas;

  i64 result = 0;
  b32 found = false;

  for (u64 font_height_index = 0;
       font_height_index < array_count(atlas->heights);
       ++font_height_index)
  {
    f32 cur_height = atlas->heights[font_height_index];

    if (approx_equal(cur_height, font_height))
    {
      found = true;
      break;
    }
    else
    {
      result += atlas->char_data_set_counts[font_height_index];
    }
  }

  return(found ? result : -1);
}

internal void render_draw_text(f32 *baseline_x, f32 *baseline_y, RGBA_f32 color, utf8 *format, ...)
{
  Arena *temp_arena = get_temp_arena();

  u64 sprinted_text_cap = 512;
  String_utf8 sprinted_text =
  {
    (utf8 *) arena_push(temp_arena, sprinted_text_cap),
    0,
    sprinted_text_cap
  };

  va_list args;
  va_start(args, format);
  sprinted_text.size = stbsp_vsnprintf((char *) sprinted_text.str, (i32) sprinted_text.cap, (char *) format, args);
  va_end(args);

  Texture_Atlas *atlas = win32_global_state.render_context.atlas;
  Instance_Buffer_Element *render_elements =
    push_array(&win32_global_state.render_context.render_data, Instance_Buffer_Element, sprinted_text.size);

  f32 font_scale = stbtt_ScaleForPixelHeight(&atlas->font_info, atlas->heights[0]);

  f32 cur_x = *baseline_x;
  f32 cur_y = *baseline_y;

  for (u64 text_index = 0;
       (sprinted_text.str[text_index] != '\0') && (text_index < sprinted_text.size);
       ++text_index)
  {
    // TODO(antonio): deal with new lines more gracefully
    if (is_newline(sprinted_text.str[text_index]))
    {
      continue;
    }
    else
    {
      Instance_Buffer_Element *cur_element     = render_elements  +  text_index;
      stbtt_packedchar        *cur_packed_char = atlas->char_data + (sprinted_text.str[text_index] - starting_code_point);

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

      cur_element->color[0] = color;
      cur_element->color[1] = color;
      cur_element->color[2] = color;
      cur_element->color[3] = color;

      f32 kern_advance = 0.0f;
      if (text_index < (sprinted_text.size - 1))
      {
        kern_advance = font_scale *
          stbtt_GetCodepointKernAdvance(&atlas->font_info,
                                        sprinted_text.str[text_index],
                                        sprinted_text.str[text_index + 1]);
      }

      cur_x += kern_advance + cur_packed_char->xadvance;
    }
  }

  *baseline_x = cur_x;
}

internal u64 platform_get_processor_time_stamp(void)
{
  u64 time_stamp = (u64) __rdtsc();
  return(time_stamp);
}

internal u64 platform_get_high_precision_timer(void)
{
  u64 result = {};

  LARGE_INTEGER high_precision_time = {};
  QueryPerformanceCounter(&high_precision_time);
  result = high_precision_time.QuadPart;

  return(result);
}

internal double platform_convert_high_precision_time_to_seconds(u64 hpt)
{
  double result = ((double) hpt) / ((double) meta_info.high_precision_timer_frequency);
  return(result);
}

#define TRADER_KEYPAD_ENTER (VK_RETURN + 256)
internal Key_Event platform_convert_key_to_our_key(u64 key_value)
{
    switch (key_value)
    {
        case VK_TAB:              return key_event_tab;
        case VK_LEFT:             return key_event_left_arrow;
        case VK_RIGHT:            return key_event_right_arrow;
        case VK_UP:               return key_event_up_arrow;
        case VK_DOWN:             return key_event_down_arrow;
        case VK_PRIOR:            return key_event_page_up;
        case VK_NEXT:             return key_event_page_down;
        case VK_HOME:             return key_event_home;
        case VK_END:              return key_event_end;
        case VK_INSERT:           return key_event_insert;
        case VK_DELETE:           return key_event_delete;
        case VK_BACK:             return key_event_backspace;
        case VK_SPACE:            return key_event_space;
        case VK_RETURN:           return key_event_enter;
        case VK_ESCAPE:           return key_event_escape;
        case VK_OEM_7:            return key_event_apostrophe;
        case VK_OEM_COMMA:        return key_event_comma;
        case VK_OEM_MINUS:        return key_event_minus;
        case VK_OEM_PERIOD:       return key_event_period;
        case VK_OEM_2:            return key_event_slash;
        case VK_OEM_1:            return key_event_semicolon;
        case VK_OEM_PLUS:         return key_event_equal;
        case VK_OEM_4:            return key_event_left_bracket;
        case VK_OEM_5:            return key_event_backslash;
        case VK_OEM_6:            return key_event_right_bracket;
        case VK_OEM_3:            return key_event_grave_accent;
        case VK_CAPITAL:          return key_event_caps_lock;
        case VK_SCROLL:           return key_event_scroll_lock;
        case VK_NUMLOCK:          return key_event_num_lock;
        case VK_SNAPSHOT:         return key_event_print_screen;
        case VK_PAUSE:            return key_event_pause;
        case VK_NUMPAD0:          return key_event_keypad_0;
        case VK_NUMPAD1:          return key_event_keypad_1;
        case VK_NUMPAD2:          return key_event_keypad_2;
        case VK_NUMPAD3:          return key_event_keypad_3;
        case VK_NUMPAD4:          return key_event_keypad_4;
        case VK_NUMPAD5:          return key_event_keypad_5;
        case VK_NUMPAD6:          return key_event_keypad_6;
        case VK_NUMPAD7:          return key_event_keypad_7;
        case VK_NUMPAD8:          return key_event_keypad_8;
        case VK_NUMPAD9:          return key_event_keypad_9;
        case VK_DECIMAL:          return key_event_keypad_decimal;
        case VK_DIVIDE:           return key_event_keypad_divide;
        case VK_MULTIPLY:         return key_event_keypad_multiply;
        case VK_SUBTRACT:         return key_event_keypad_subtract;
        case VK_ADD:              return key_event_keypad_add;
        case TRADER_KEYPAD_ENTER: return key_event_keypad_enter;
        case VK_LSHIFT:           return key_event_left_shift;
        case VK_LCONTROL:         return key_event_left_ctrl;
        case VK_LMENU:            return key_event_left_alt;
        case VK_LWIN:             return key_event_left_super;
        case VK_RSHIFT:           return key_event_right_shift;
        case VK_RCONTROL:         return key_event_right_ctrl;
        case VK_RMENU:            return key_event_right_alt;
        case VK_RWIN:             return key_event_right_super;
        case VK_APPS:             return key_event_menu;
        case '0':                 return key_event_0;
        case '1':                 return key_event_1;
        case '2':                 return key_event_2;
        case '3':                 return key_event_3;
        case '4':                 return key_event_4;
        case '5':                 return key_event_5;
        case '6':                 return key_event_6;
        case '7':                 return key_event_7;
        case '8':                 return key_event_8;
        case '9':                 return key_event_9;
        case 'A':                 return key_event_a;
        case 'B':                 return key_event_b;
        case 'C':                 return key_event_c;
        case 'D':                 return key_event_d;
        case 'E':                 return key_event_e;
        case 'F':                 return key_event_f;
        case 'G':                 return key_event_g;
        case 'H':                 return key_event_h;
        case 'I':                 return key_event_i;
        case 'J':                 return key_event_j;
        case 'K':                 return key_event_k;
        case 'L':                 return key_event_l;
        case 'M':                 return key_event_m;
        case 'N':                 return key_event_n;
        case 'O':                 return key_event_o;
        case 'P':                 return key_event_p;
        case 'Q':                 return key_event_q;
        case 'R':                 return key_event_r;
        case 'S':                 return key_event_s;
        case 'T':                 return key_event_t;
        case 'U':                 return key_event_u;
        case 'V':                 return key_event_v;
        case 'W':                 return key_event_w;
        case 'X':                 return key_event_x;
        case 'Y':                 return key_event_y;
        case 'Z':                 return key_event_z;
        case VK_F1:               return key_event_f1;
        case VK_F2:               return key_event_f2;
        case VK_F3:               return key_event_f3;
        case VK_F4:               return key_event_f4;
        case VK_F5:               return key_event_f5;
        case VK_F6:               return key_event_f6;
        case VK_F7:               return key_event_f7;
        case VK_F8:               return key_event_f8;
        case VK_F9:               return key_event_f9;
        case VK_F10:              return key_event_f10;
        case VK_F11:              return key_event_f11;
        case VK_F12:              return key_event_f12;
        default:                  return key_event_none;
    }
}

internal String_Const_utf8 platform_get_file_from_system_prompt()
{
  Arena *temp_arena = get_temp_arena();

  u32 string_size = 256;
  String_utf16 file_name_buffer = push_string_zero(temp_arena, utf16, string_size);
  String_utf16 file_path  = push_string_zero(temp_arena, utf16, string_size);

  OPENFILENAMEW open_file;

  open_file.lStructSize       = sizeof(open_file);
  open_file.hwndOwner         = NULL;
  open_file.hInstance         = NULL;
  open_file.lpstrFilter       = NULL;
  open_file.lpstrCustomFilter = NULL;
  open_file.nMaxCustFilter    = 0;
  open_file.nFilterIndex      = 0;
  open_file.lpstrFile         = (LPWSTR) file_path.str;
  open_file.nMaxFile          = string_size;
  open_file.lpstrFileTitle    = (LPWSTR) file_name_buffer.str;
  open_file.nMaxFileTitle     = string_size;
  open_file.lpstrInitialDir   = NULL;
  open_file.lpstrTitle        = NULL;

  // TODO(antonio): OFN_EXPLORER 
  open_file.Flags             = //OFN_CREATEPROMPT | OFN_FORCESHOWHIDDEN |
                                //OFN_HIDEREADONLY | OFN_LONGNAMES       |
                                OFN_PATHMUSTEXIST;

  open_file.nFileOffset       = 0;
  open_file.nFileExtension    = 0;
  open_file.lpstrDefExt       = 0; 
  open_file.lCustData         = NULL;
  open_file.lpfnHook          = NULL;
  open_file.lpTemplateName    = NULL;                                         
  // NOTE(antonio): literally insane
#ifdef _MAC
  open_file.lpEditInfo        = 0;
  open_file.lpstrPrompt       = 0;
#endif
  open_file.pvReserved        = 0;
  open_file.dwReserved        = 0;
  open_file.FlagsEx           = 0;

  String_Const_utf8 res = {};
  if(GetOpenFileNameW(&open_file))
  {
    String_utf8 temp = push_string_zero(temp_arena, utf8, string_size);
    res.size = WideCharToMultiByte(CP_UTF8,
                                   0,
                                   (LPWSTR) file_path.str,
                                   (i32) c_string_length((wchar_t *) file_path.str),
                                   (LPSTR) temp.str,
                                   (i32) temp.cap,
                                   NULL, NULL);
    res.str = temp.str;
  }

  return(res);
}

internal File_Buffer platform_open_and_read_entire_file_from_system_prompt(Arena *arena)
{
  File_Buffer res = {};
  String_Const_utf8 file_path = platform_get_file_from_system_prompt();
  if (file_path.str != NULL)
  {
    res = platform_open_and_read_entire_file(arena, file_path.str, file_path.size);
  }
  return(res);
}

#define WIN32_IMPLEMENTATION_H
#endif
