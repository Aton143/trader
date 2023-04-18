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

struct Socket
{
  SOCKET                    socket;

  CredHandle                cred_handle;
  CtxtHandle                security_context;

  SecPkgContext_StreamSizes sizes;

  i32                       received;    // byte count in incoming buffer (ciphertext)
  i32                       used;        // byte count used from incoming buffer to decrypt current packet
  i32                       available;   // byte count available for decrypted bytes
  u8                       *decrypted;   // points to incoming buffer where data is decrypted inplace
  u8                        incoming[TLS_MAX_PACKET_SIZE];
};
Socket nil_socket = {INVALID_SOCKET, NULL, NULL, {}, 0, 0, 0, NULL};

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

internal Network_Return_Code network_startup()
{
  Network_Return_Code result = network_ok;

  WSADATA winsock_metadata = {};
  i32 winsock_result = WSAStartup(MAKEWORD(2, 2), &winsock_metadata);
  assert((winsock_result == 0) && "expected winsock dll to load");

  return(result);
}

Network_Return_Code network_connect(String_Const_utf8 host_name, u16 port, Socket *out_socket)
{
  Network_Return_Code result = network_ok;

  assert(out_socket    != NULL);
  assert(host_name.str != NULL);

  copy_struct(out_socket, &nil_socket);

  CtxtHandle* security_context = NULL;

  out_socket->socket = socket(AF_INET, SOCK_STREAM, 0);

  u8 port_name[64] = {};
  stbsp_snprintf((char *) port_name, sizeof(port_name), "%u", port);

  // TODO(antonio): ansi versions are deprecated
  b32 connected = WSAConnectByNameA(out_socket->socket,
                                    (char *) host_name.str,
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
                                &out_socket->cred_handle,
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

  for (;;)
  {
    SecBuffer in_buffers[2] = {};
    {
      in_buffers[0].BufferType = SECBUFFER_TOKEN;
      in_buffers[0].pvBuffer   = out_socket->incoming;
      in_buffers[0].cbBuffer   = out_socket->received;

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

    SECURITY_STATUS sec = InitializeSecurityContextA(&out_socket->cred_handle,
                                                     security_context,
                                                     security_context ? NULL : (SEC_CHAR *) host_name.str,
                                                     security_init_flags,
                                                     0,
                                                     0,
                                                     security_context ? &in_desc : NULL,
                                                     0,
                                                     security_context ? NULL : &out_socket->security_context,
                                                     &out_desc,
                                                     &security_init_flags,
                                                     NULL);

    // NOTE(antonio): after first call to InitializeSecurityContextA, context will be available and can be reused 
    security_context = &out_socket->security_context;

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
      move_memory_block(out_socket->incoming,
                        out_socket->incoming + (out_socket->received - in_buffers[1].cbBuffer),
                        in_buffers[1].cbBuffer);
      out_socket->received = in_buffers[1].cbBuffer;
    }
    else
    {
      out_socket->received = 0;
    }

    if (sec == SEC_E_OK)
    {
      // NOTE(antonio): tls handshake completed
      break;
    }
    else if (sec == SEC_I_INCOMPLETE_CREDENTIALS)
    {
      // NOTE(antonio): server asked for client certificate, not supported yet
      result = network_error_client_certificate_needed;
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
        i32 bytes_sent = send(out_socket->socket, (char *) output_token, token_size, 0);

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
        result = network_error_send_failure;
        assert(!"failed to send data to server");
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
      result = network_error_unknown;
      assert(!"unimplemented");
      break;
    }

    // read more data from server when possible
    if (out_socket->received == sizeof(out_socket->incoming))
    {
      result = network_error_unknown;

      network_disconnect(out_socket);
      copy_struct(out_socket, &nil_socket);

      break;
    }

    i32 bytes_received = recv(out_socket->socket,
                              (char *) (out_socket->incoming + out_socket->received),
                              sizeof(out_socket->incoming) - out_socket->received,
                              0);
    if (bytes_received == 0)
    {
      result = network_error_socket_disconnected;

      network_disconnect(out_socket);
      copy_struct(out_socket, &nil_socket);

      break;
    }
    else if (bytes_received < 0)
    {
      result = network_error_socket_error;

      assert("is this consistent?");
      int error = WSAGetLastError();  
      unused(error);

      break;
    }

    if (result != 0)
    {
      DeleteSecurityContext(security_context);
      FreeCredentialsHandle(&out_socket->cred_handle);

      closesocket(out_socket->socket);
    }
    else
    {
      QueryContextAttributes(security_context, SECPKG_ATTR_STREAM_SIZES, &out_socket->sizes);
    }

    out_socket->received += bytes_received;
  }

  return(result);
}

Network_Return_Code network_send(Socket *in_socket, Buffer to_send)
{
  Network_Return_Code result = network_ok;

  assert(to_send.data != NULL);

  u64 send_size = (u64) to_send.used;

  if (compare_struct_shallow(in_socket, &nil_socket) != 0)
  {
    // NOTE(antonio): encrypt and send
    while (send_size != 0)
    {
      u32 use = (u32) min(send_size, in_socket->sizes.cbMaximumMessage);

      char _wbuffer[TLS_MAX_PACKET_SIZE];
      Buffer wbuffer = buffer_from_fixed_size(_wbuffer);

      u64 max_size = in_socket->sizes.cbHeader +
        in_socket->sizes.cbMaximumMessage +
        in_socket->sizes.cbTrailer;

      assert(max_size <= wbuffer.size);

      SecBuffer sec_buffers[3];
      {
        sec_buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        sec_buffers[0].pvBuffer   = wbuffer.data;
        sec_buffers[0].cbBuffer   = in_socket->sizes.cbHeader;

        sec_buffers[1].BufferType = SECBUFFER_DATA;
        sec_buffers[1].pvBuffer   = wbuffer.data + in_socket->sizes.cbHeader;
        sec_buffers[1].cbBuffer   = use;

        sec_buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        sec_buffers[2].pvBuffer   = wbuffer.data + in_socket->sizes.cbHeader + use;
        sec_buffers[2].cbBuffer   = in_socket->sizes.cbTrailer;
      }

      copy_memory_block(sec_buffers[1].pvBuffer, to_send.data, use);

      SecBufferDesc sec_buffer_description = {SECBUFFER_VERSION, array_count(sec_buffers), sec_buffers};
      SECURITY_STATUS sec = EncryptMessage(&in_socket->security_context, 0, &sec_buffer_description, 0);
      if (sec != SEC_E_OK)
      {
        // this should not happen, but just in case check it
        result = network_error_unknown; 
        break;
      }

      i32 total_buffer_size = sec_buffers[0].cbBuffer + sec_buffers[1].cbBuffer + sec_buffers[2].cbBuffer;
      i32 sent = 0;
      while (sent != total_buffer_size)
      {
        i32 bytes_left_to_send = total_buffer_size - sent;
        u8 *buffer_start = wbuffer.data + sent;

        i32 bytes_sent = send(in_socket->socket, (char *) buffer_start, bytes_left_to_send, 0);
        if (bytes_sent <= 0)
        {
          // NOTE(antonio): error sending data to socket, or server disconnected
          result = network_error_socket_error;
          break;
        }

        sent += bytes_sent;
      }

      to_send.data = to_send.data + use;
      send_size -= use;
    }
  }

  return(result);
}

internal Network_Return_Code network_receive(Socket *in_socket, Buffer *out_receive_buffer)
{
  Network_Return_Code result = network_ok;

  assert(in_socket          != NULL);
  assert(out_receive_buffer != NULL);

  if (compare_struct_shallow(in_socket, &nil_socket) != 0)
  {
    i64 read_result = 0;
    while (out_receive_buffer->used < out_receive_buffer->size)
    {
      if (in_socket->decrypted != NULL)
      {
        // NOTE(antonio): if there is decrypted data available, then use it as much as possible
        i32 use = (i32) min(out_receive_buffer->size, in_socket->available);

        copy_memory_block(&out_receive_buffer->data[out_receive_buffer->used], in_socket->decrypted, use);

        out_receive_buffer->used += use;
        read_result += use;

        if (use == in_socket->available)
        {
          // NOTE(antonio):
          // all decrypted data is used, remove ciphertext from
          // incoming buffer so next time it starts from beginning
          move_memory_block(in_socket->incoming,
                            in_socket->incoming + in_socket->used,
                            in_socket->received - in_socket->used);

          in_socket->received  -= in_socket->used;
          in_socket->used       = 0;
          in_socket->available  = 0;
          in_socket->decrypted  = NULL;
        }
        else
        {
          in_socket->available -= use;
          in_socket->decrypted += use;
        }
      }
      else
      {
        // NOTE(antonio): if any ciphertext data available then try to decrypt it
        if (in_socket->received != 0)
        {
          SecBuffer sec_buffers[4];
          {
            assert(in_socket->sizes.cBuffers == array_count(sec_buffers));

            sec_buffers[0].BufferType = SECBUFFER_DATA;
            sec_buffers[0].pvBuffer = in_socket->incoming;
            sec_buffers[0].cbBuffer = in_socket->received;

            sec_buffers[1].BufferType = SECBUFFER_EMPTY;
            sec_buffers[2].BufferType = SECBUFFER_EMPTY;
            sec_buffers[3].BufferType = SECBUFFER_EMPTY;
          }

          SecBufferDesc sec_buffer_description = {SECBUFFER_VERSION, array_count(sec_buffers), sec_buffers};

          SECURITY_STATUS sec = DecryptMessage(&in_socket->security_context,
                                               &sec_buffer_description,
                                               0, NULL);
          if (sec == SEC_E_OK)
          {
            assert(sec_buffers[0].BufferType == SECBUFFER_STREAM_HEADER);
            assert(sec_buffers[1].BufferType == SECBUFFER_DATA);
            assert(sec_buffers[2].BufferType == SECBUFFER_STREAM_TRAILER);

            u64 used = in_socket->received -
              (sec_buffers[3].BufferType == SECBUFFER_EXTRA ?
               sec_buffers[3].cbBuffer : 0);

            in_socket->decrypted = (u8 *) sec_buffers[1].pvBuffer;
            in_socket->available = sec_buffers[1].cbBuffer;
            in_socket->used      = (i32) used;

            // NOTE(antonio):
            // data is now decrypted, go back to beginning
            // of loop to copy memory to output buffer
            continue;
          }
          else if (sec == SEC_I_CONTEXT_EXPIRED)
          {
            // NOTE(antonio): server closed TLS connection (but socket is still open)
            result = network_error_socket_disconnected;
            in_socket->received = 0;
          }
          else if (sec == SEC_I_RENEGOTIATE)
          {
            // TODO(antonio): server wants to renegotiate TLS connection, not implemented here
            assert(!"unimplemented");
          }
          else if (sec != SEC_E_INCOMPLETE_MESSAGE)
          {
            // NOTE(antonio): some other schannel or TLS protocol error
            result = network_error_unknown;
            break;
          }
          // NOTE(antonio): otherwise sec == SEC_E_INCOMPLETE_MESSAGE which means need to read more data
        }
      }
      // otherwise not enough data received to decrypt

      if (read_result != 0)
      {
        // some data is already copied to output buffer, so return that before blocking with recv
        break;
      }

      if (in_socket->received == sizeof(in_socket->incoming))
      {
        assert(!"server is sending too much garbage data instead of proper TLS packet");
      }

      // wait for more ciphertext data from server
      i32 bytes_received = recv(in_socket->socket,
                                (char *) (in_socket->incoming + in_socket->received),
                                sizeof(in_socket->incoming) - in_socket->received,
                                0);
      if (bytes_received == 0)
      {
        result = network_error_socket_disconnected;
        break;
      }
      else if (bytes_received < 0)
      {
        result = network_error_receive_failure;
        break;
      }

      in_socket->received += bytes_received;
    }
  }

  return(result);
}

internal Network_Return_Code network_disconnect(Socket *in_out_socket)
{
  Network_Return_Code result = network_ok;

  assert(in_out_socket != NULL);

  DWORD shutdown_type = SCHANNEL_SHUTDOWN;

  SecBuffer in_buffers[1];
  {
    in_buffers[0].BufferType = SECBUFFER_TOKEN;
    in_buffers[0].pvBuffer   = &shutdown_type;
    in_buffers[0].cbBuffer   = sizeof(shutdown_type);
  }

  SecBufferDesc desc = {SECBUFFER_VERSION, array_count(in_buffers), in_buffers};
  ApplyControlToken(&in_out_socket->security_context, &desc);

  SecBuffer out_buffers[1];
  {
    out_buffers[0].BufferType = SECBUFFER_TOKEN;
  }

  desc = {SECBUFFER_VERSION, array_count(out_buffers), out_buffers};
  DWORD flags = ISC_REQ_ALLOCATE_MEMORY |
                ISC_REQ_CONFIDENTIALITY |
                ISC_REQ_REPLAY_DETECT |
                ISC_REQ_SEQUENCE_DETECT |
                ISC_REQ_STREAM;

  SECURITY_STATUS status =
    InitializeSecurityContextA(&in_out_socket->cred_handle, &in_out_socket->security_context,
                               NULL, flags, 0, 0, &desc, 0, NULL, &desc, &flags, NULL);
  if (status == SEC_E_OK)
  {
    char *buffer = (char *) out_buffers[0].pvBuffer;
    i32 size = out_buffers[0].cbBuffer;
    while (size != 0)
    {
      i32 synthetic_bytes_sent = send(in_out_socket->socket, buffer, size, 0);
      if (synthetic_bytes_sent <= 0)
      {
        // ignore any failures socket will be closed anyway
        break;
      }

      buffer += synthetic_bytes_sent;
      size -= synthetic_bytes_sent;
    }

    FreeContextBuffer(out_buffers[0].pvBuffer);
  }

  shutdown(in_out_socket->socket, SD_BOTH);

  DeleteSecurityContext(&in_out_socket->security_context);
  FreeCredentialsHandle(&in_out_socket->cred_handle);
  closesocket(in_out_socket->socket);

  copy_struct(in_out_socket, &nil_socket);

  return(result);
}

internal Network_Return_Code network_cleanup()
{
  Network_Return_Code result = network_ok;

  WSACleanup();

  return(result);
}

#define WIN32_IMPLEMENTATION_H
#endif
