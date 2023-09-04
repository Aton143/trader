#ifndef WIN32_IMPLEMENTATION_H
#include <malloc.h>

#include "../trader_platform.h"
#include "../trader_ui.h"
#include "../trader_render.h"
#include "../trader_network.h"
#include "../trader_math.h"
#include "../trader_unicode.h"

struct Thread_Handle 
{
  HANDLE _handle;
};

struct Cursor_Handle
{
  HCURSOR _handle;
};
Cursor_Handle cursors[cursor_kind_count];

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
  union
  {
    struct
    {
      Texture_Atlas *atlas;
      Arena          render_data;
      Arena          triangle_render_data;
      V2_f32         vertex_render_dimensions;
      Rect_f32       client_rect;
      File_Buffer    default_font;
    };
    Common_Render_Context common_context;
  };

  Texture_Atlas       *wingding;
  IDXGISwapChain1     *swap_chain;
  ID3D11Device        *device;
  ID3D11DeviceContext *device_context;
};

#pragma pack(push, 4)
struct Global_Platform_State
{
  Temp_Arena      temp_arena;
  Arena           global_arena;

  Render_Context  render_context;
  UI_Context      ui_context;

  Focus_Event     focus_event;
  f64             dt;

  HWND            window_handle;

  HANDLE          notify_iocp;
  HANDLE          notify_dir;
  HANDLE          notify_dir_iocp;
  OVERLAPPED      notify_overlapped;

  void           *main_fiber_address;
  u64             nonclient_mouse_pos;
  u32             nonclient_mouse_button;

  /*
  HCURSOR         horizontal_resize_cursor_icon;
  HCURSOR         vertical_resize_cursor_icon;
  */

#if !SHIP_MODE
  u64 frame_count;
#endif

  // TODO(antonio): make part of global arena
  u8             _changed_files[kb(1)];
  utf8           changed_files[8][128];

  b8              running;
  b8              window_resized;
};
#pragma pack(pop)

global Global_Platform_State win32_global_state  = {};
global_const String_Const_utf8 default_font_path =
  string_literal_init_type("C:/windows/fonts/arial.ttf", utf8);

global_const u8 platform_path_separator = '\\';
global_const u8 unix_path_separator = '/';
global_const f32 default_font_heights[] = {24.0f};

internal Render_Context *render_get_context(void)
{
  Render_Context *context = &win32_global_state.render_context;
  return(context);
}

internal Arena *platform_get_global_arena()
{
  return(&platform_get_global_state()->global_arena);
}

struct Socket
{
  SOCKET socket;
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

internal void meta_init(void)
{
  LARGE_INTEGER hpt_freq;
  QueryPerformanceFrequency(&hpt_freq);
  meta_info.high_precision_timer_frequency = hpt_freq.QuadPart;

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
      type_pun(HANDLE, out_handle->file_handle) = file_handle;
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
      type_pun(HANDLE, out_handle->file_handle) = file_handle;
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
        file_buffer.used           = file_size;
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
            file_buffer.used           = file_size;

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
  unused(out_state);

  Network_Return_Code result = network_ok;

  WSADATA winsock_metadata = {};
  i32 winsock_result = WSAStartup(MAKEWORD(2, 2), &winsock_metadata);
  expect_message(winsock_result == 0, "expected winsock dll to load");

  return(result);
}

internal Network_Return_Code network_connect(Network_State *state, Socket *out_socket, String_Const_utf8 host_name, u16 port)
{
  unused(state);
  unused(out_socket);
  unused(host_name);
  unused(port);

  Network_Return_Code result = network_ok;

  expect(out_socket    != NULL);
  expect(host_name.str != NULL);

  make_nil(out_socket);

  utf8 port_name[64] = {};
  stbsp_snprintf((char *) port_name, sizeof(port_name), "%u", port);

  addrinfo addr_hints = {};
  {
    addr_hints.ai_family   = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_STREAM;
  }

  addrinfo *first_addr;

  i32 found_addr = getaddrinfo((char *) host_name.str, (char *) port_name, &addr_hints, &first_addr);
  expect_message((found_addr == 0) && (first_addr != NULL), "could not get address info for the given host name");

  SOCKET found_socket = INVALID_SOCKET;
  for (addrinfo *cur_addr = first_addr;
       cur_addr != NULL;
       cur_addr = cur_addr->ai_next)
  {
    found_socket = WSASocket(cur_addr->ai_family,
                             cur_addr->ai_socktype,
                             cur_addr->ai_protocol,
                             NULL,
                             0,
                             WSA_FLAG_OVERLAPPED); 

    if (found_socket != INVALID_SOCKET)
    {
      out_socket->socket = found_socket;
      break;
    }
    else
    {
      closesocket(found_socket);
      found_socket = INVALID_SOCKET;
    }
  }

  expect_message(found_socket != INVALID_SOCKET, "expected connection");
  freeaddrinfo(first_addr);

  b32 connected = WSAConnectByNameA(out_socket->socket,
                                   (char *) host_name.str, (char *) port_name,
                                   0, NULL, 0, NULL, NULL, NULL);
  expect_message(connected, "expected connection");

  return(result);
}

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *send_buffer)
{
  Network_Return_Code result = network_ok;

  unused(state);
  expect(send_buffer->data != NULL);

  if (!is_nil(in_socket))
  {
  }

  return(result);
}

internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive_buffer)
{
  unused(state);
  unused(receive_buffer);

  Network_Return_Code result = network_ok;

  expect(in_socket      != NULL);
  expect(receive_buffer != NULL);

  i32 bytes_received;
  unused(bytes_received);

  if (!is_nil(in_socket))
  {
    WSAPOLLFD sockets_to_poll[1] = {};

    sockets_to_poll[0].fd     = in_socket->socket;
    sockets_to_poll[0].events = POLLIN;

    i32 poll_res = WSAPoll(sockets_to_poll, array_count(sockets_to_poll), -1);

    if (poll_res == 0)
    {
      result = network_no_data;
    }

    while (poll_res > 0)
    {
    }
  }

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

  Buffer websocket_handshake_send = stack_alloc_buffer(1024);

  websocket_handshake_send.used =
    stbsp_snprintf((char *) websocket_handshake_send.data, (int) websocket_handshake_send.size,
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

  Buffer websocket_receive = stack_alloc_buffer(kb(4));
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

internal void platform_debug_print(char *text)
{
  OutputDebugStringA(text);
}

internal void platform_debug_print_system_error()
{
  DWORD error = GetLastError();

  utf16 *return_buffer = NULL;
  DWORD  return_buffer_length =
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  error,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) return_buffer,
                  0,
                  NULL);

  if (return_buffer_length > 0)
  {
    OutputDebugStringW(return_buffer);
    LocalFree(return_buffer);
  }
}

internal void *render_load_vertex_shader(Handle *shader_handle, Vertex_Shader *shader, b32 force)
{
  void *blob = NULL;

  if (force || platform_did_file_change(shader_handle->id.str, shader_handle->id.size))
  {
    safe_release(shader->shader);

    // TODO(antonio): will have to read file twice
    File_Buffer temp_shader_source = platform_read_entire_file(shader_handle);
    {
      ID3DBlob *vertex_shader_blob = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      UINT compile_flags = 0;
#if !SHIP_MODE
      compile_flags |= D3DCOMPILE_DEBUG;
      compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

      HRESULT result = D3DCompile(temp_shader_source.data, temp_shader_source.size, NULL, NULL, NULL,
                                  "VS_Main", "vs_5_0", compile_flags, 0, &vertex_shader_blob, &shader_compile_errors_blob);

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
  if (force || platform_did_file_change(shader_handle->id.str, shader_handle->id.size))
  {
    safe_release(shader->shader);

    // TODO(antonio): will have to read file twice: once for vertex, again for pixel
    File_Buffer temp_shader_source = platform_read_entire_file(shader_handle);
    {
      ID3DBlob *pixel_shader_blob          = NULL;
      ID3DBlob *shader_compile_errors_blob = NULL;

      UINT compile_flags = 0;
#if !SHIP_MODE
      compile_flags |= D3DCOMPILE_DEBUG;
      compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

      HRESULT return_code = D3DCompile(temp_shader_source.data, temp_shader_source.size, NULL, NULL, NULL,
                                       "PS_Main", "ps_5_0", compile_flags, 0, &pixel_shader_blob, &shader_compile_errors_blob);

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
    if (approx_equal_f32(cur_height, font_height))
    {
      result = font_height_index;
    }
  }

  return(result);
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

internal Thread_Handle platform_create_thread(Thread_Routine routine, void *routine_arg)
{
  Thread_Handle res = {};
  res._handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) routine, routine_arg, 0, NULL);
  return(res);
}

internal void platform_set_cursor(Cursor_Kind cursor)
{
  expect(cursor < cursor_kind_count);
  expect(cursors[cursor]._handle != NULL);
  SetCursor(cursors[cursor]._handle);
}

internal String_Const_utf8 platform_read_clipboard_contents(Arena *arena)
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

          result.str = unicode_utf8_from_utf16(arena, clip_utf16.str, clip_utf16.size, (i64 *) &result.size);

          got_result = true;
        }
        GlobalUnlock(clip_data);
      }
      else
      {
        platform_debug_print_system_error();
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

          copy_struct(&result, &clip_char);

          got_result = true;
        }
        GlobalUnlock(clip_data);
      }
    }

    if (result.str == NULL)
    {
      platform_debug_print_system_error();
    }

    CloseClipboard();
  }

  return(result);
}

internal void platform_write_clipboard_contents(String_utf8 string)
{
  if (OpenClipboard(win32_global_state.window_handle))
  {
    if (EmptyClipboard())
    {
      HGLOBAL data_handle = GlobalAlloc(GMEM_MOVEABLE, string.size + 1);
      platform_debug_print_system_error();

      if (data_handle != NULL)
      {
        void *write_dest = GlobalLock(data_handle);
        platform_debug_print_system_error();

        if (write_dest != NULL)
        {
          copy_memory_block(write_dest, string.str, string.size);
          *(((u8 * ) write_dest) + string.size) = 0;

          if (GlobalUnlock(data_handle) == NULL)
          {
            if (SetClipboardData(CF_TEXT, data_handle) == NULL)
            {
              platform_debug_print_system_error();
            }
          }
          else
          {
            platform_debug_print_system_error();
          }
        }
        else
        {
          platform_debug_print_system_error();
        }
      }
    }
    else
    {
      platform_debug_print_system_error();
    }

    CloseClipboard();
  }
  else
  {
    platform_debug_print_system_error();
  }

  platform_debug_print_system_error();
}

internal u8 *platform_allocate_memory_pages(u64 bytes, void *start)
{
  u8 *pages = (u8 *) VirtualAlloc(start, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  return(pages);
}

internal u64 platform_get_processor_time_stamp(void)
{
  u64 ts = (u64) __rdtsc();
  return(ts);
}

internal void win32_load_skybox(Bitmap *bitmaps)
{
  Arena *temp_arena = get_temp_arena();
  u64 prev_used = temp_arena->used;

  local_persist String_Const_utf8 skybox_file_paths[]
  {
    scu8l("..\\assets\\skybox\\right.jpg"),
    scu8l("..\\assets\\skybox\\left.jpg"),
    scu8l("..\\assets\\skybox\\top.jpg"),
    scu8l("..\\assets\\skybox\\bottom.jpg"),
    scu8l("..\\assets\\skybox\\front.jpg"),
    scu8l("..\\assets\\skybox\\back.jpg"),
  };

  for (u32 path_index = 0;
       path_index < array_count(skybox_file_paths);
       ++path_index)
  {
    Bitmap *cur_bitmap = bitmaps + path_index;

    String_Const_utf8 *cur_path = skybox_file_paths + path_index;
    File_Buffer        cur_file_buf = platform_open_and_read_entire_file(temp_arena, cur_path->str, cur_path->size);

    int width, height;

    u8 *data = (u8 *) stbi_load_from_memory(cur_file_buf.data, (int) cur_file_buf.used,
                                            &width, &height, (int *) NULL, 4);

    cur_bitmap->channels = 4;
    cur_bitmap->width    = (f32) width;
    cur_bitmap->height   = (f32) height;

    /*
    u8 *image = (u8 *) arena_push(&win32_global_state.global_arena, 4 * sizeof(u8) * width * height);

    u32 data_pos  = 0;
    u32 image_pos = 0;

    for (i32 row = 0; row < height; ++row)
    {
      for (i32 col = 0; col < width; ++col)
      {
        image[image_pos++] = 255;
        image[image_pos++] = data[data_pos++];
        image[image_pos++] = data[data_pos++];
        image[image_pos++] = data[data_pos++];
      }
    }
    */

    cur_bitmap->data = data;
    temp_arena->used = prev_used;
  }
}

internal void render_create_cubemap(Bitmap *bitmaps, u32 bitmap_count, void *out_textures)
{
  expect(bitmap_count > 0); 

  ID3D11ShaderResourceView **texture_views = (ID3D11ShaderResourceView **) out_textures;
  ID3D11Texture2D *cube_texture = NULL;

  Render_Context *render = render_get_context();

  D3D11_TEXTURE2D_DESC texture_desc = {};
  {
    texture_desc.Width              = (UINT) bitmaps->width;
    texture_desc.Height             = (UINT) bitmaps->height;
    texture_desc.MipLevels          = 1;
    texture_desc.ArraySize          = 6;
    texture_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM ;
    texture_desc.CPUAccessFlags     = 0;
    texture_desc.SampleDesc.Count   = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage              = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags     = 0;
    texture_desc.MiscFlags          = D3D11_RESOURCE_MISC_TEXTURECUBE;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  {
    srv_desc.Format                      = texture_desc.Format;
    srv_desc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MipLevels       = texture_desc.MipLevels;
    srv_desc.TextureCube.MostDetailedMip = 0;
  }

  D3D11_SUBRESOURCE_DATA subresource_data[6] = {};
  for (u32 face_index = 0;
       face_index < array_count(subresource_data);
       ++face_index)
  {
    subresource_data[face_index].pSysMem          = bitmaps[face_index].data; 
    subresource_data[face_index].SysMemPitch      = ((UINT) bitmaps[face_index].width) * bitmaps[face_index].channels; 
    subresource_data[face_index].SysMemSlicePitch = 0;
  }

  HRESULT result = render->device->CreateTexture2D(&texture_desc, subresource_data, &cube_texture);
  expect(SUCCEEDED(result));

  result = render->device->CreateShaderResourceView(cube_texture, &srv_desc, texture_views);
  expect(SUCCEEDED(result));
}

internal b32 is_vk_down(i32 vk)
{
  b32 vk_down = (GetKeyState(vk) & 0x8000) != 0;
  return(vk_down);
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
        win32_global_state.running = false;
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
    case WM_INPUT:
    {
      // NOTE(antonio): https://gist.github.com/luluco250/ac79d72a734295f167851ffdb36d77ee 
      // RAWINPUT does not guarantee that accumulated deltas will be consistent, unlike WM_MOUSEDATA
      // This requires some care

      local_persist RAWINPUT raw_input[1];
      u32 size = sizeof(raw_input);
      HRAWINPUT raw_input_handle = (HRAWINPUT) lparam;

      UINT return_code = GetRawInputData(raw_input_handle, RID_INPUT, raw_input, &size, sizeof(RAWINPUTHEADER));
      if (return_code == (UINT) -1)
      {
        platform_debug_print_system_error();
      }

      if (raw_input->header.dwType == RIM_TYPEMOUSE)
      {
        RAWMOUSE *mouse_data = &raw_input->data.mouse;

        if ((mouse_data->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)
        {
          b32 virtual_desktop = (mouse_data->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;

          f32 width  = (f32) GetSystemMetrics(virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
          f32 height = (f32) GetSystemMetrics(virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

          global_player_context.mouse_pos = V2((mouse_data->lLastX / 65535.0f) * width,
                                               (mouse_data->lLastY / 65535.0f) * height);
        }
        else if ((mouse_data->usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE)
        {
          V2_f32 mouse_delta = V2((f32) mouse_data->lLastX, (f32) mouse_data->lLastY);

          global_player_context.mouse_delta = mouse_delta;
          global_player_context.mouse_pos   = add(global_player_context.mouse_pos, mouse_delta);
        }
      }
    } break;

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
      // b32 was_key_down = (((lparam >> 30)) & 1) == 0 ;

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
          player_add_input(key, is_key_down);
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
        win32_global_state.running = false;
      }
      result = DefWindowProc(window_handle, message, wparam, lparam);
    } break;

    case WM_SIZE:
    {
      win32_global_state.window_resized = true;
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
      win32_global_state.running = false;
    } break;

    default:
    {
      result = DefWindowProcW(window_handle, message, wparam, lparam);
    } break;
  }

  return(result);
}

#define WIN32_IMPLEMENTATION_H
#endif
