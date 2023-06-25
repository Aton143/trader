#ifndef TRADER_NETWORK_H

struct Network_State
{
  SSL_CTX *ssl_context;
};

#define SSL_MAX_HOST_NAME_LENGTH 255
#define SSL_OK                   1
#define SSL_ERROR                0

enum
{
  network_ok,

  network_error_client_certificate_needed,

  network_error_send_failure,
  network_error_send_unknown,

  network_error_socket_disconnected,
  network_error_socket_error,
  network_error_nil_socket,

  network_error_receive_failure,
  network_error_receive_unknown,

  network_error_encrypt_failure,
  network_error_decrypt_failure,

  network_error_unknown,

  network_return_code_count,
};
typedef i32 Network_Return_Code; 

typedef u8 WebSocket_Opcode;
enum 
{
  websocket_opcode_continuation = 0x0,
  websocket_opcode_text,
  websocket_opcode_binary,
  websocket_opcode_reserved_3,
  websocket_opcode_reserved_4,
  websocket_opcode_reserved_5,
  websocket_opcode_reserved_6,
  websocket_opcode_reserved_7,
  websocket_opcode_close,
  websocket_opcode_ping,
  websocket_opcode_pong,
  websocket_opcode_reserved_b,
  websocket_opcode_reserved_c,
  websocket_opcode_reserved_d,
  websocket_opcode_reserved_e,
  websocket_opcode_reserved_f,

  websocket_opcode_count,
};

struct WebSocket_Frame_Header
{
  u8               fin:    1;
  u8               rsv:    3;
  WebSocket_Opcode opcode: 4;

  u8 mask:   1;
  u8 length: 7;
};

external struct Socket;
external Socket nil_socket;

internal void make_nil(Socket *check);
internal b32 is_nil(Socket *check);

internal Network_Return_Code network_startup(Network_State *out_state);
internal Network_Return_Code network_cleanup(Network_State *out_state);

internal Network_Return_Code network_connect(Network_State *state, Socket *out_socket, String_Const_utf8 host_name, u16 port);
internal Network_Return_Code network_disconnect(Network_State *state, Socket *in_out_socket);

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *send);
internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive);

internal Network_Return_Code network_websocket_send_simple(Network_State    *state,
                                                           Socket           *in_socket,
                                                           Buffer           *send,
                                                           WebSocket_Opcode  op = websocket_opcode_text);

internal Network_Return_Code network_websocket_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive);

internal Network_Return_Code network_do_websocket_handshake(Network_State *state,
                                                            Socket *in_out_socket,
                                                            String_Const_utf8 host_name,
                                                            String_Const_utf8 query_path);

internal void network_print_error();

// implementation
internal void network_print_error()
{
  local_persist u8 network_error_buffer[512] = {};
  zero_literal(network_error_buffer);

  u32 ssl_error = ERR_get_error();
  ERR_error_string_n(ssl_error, (char *) network_error_buffer, array_count(network_error_buffer) - 1);
  platform_debug_print((char *) network_error_buffer);
}

internal Network_Return_Code network_websocket_send_simple(Network_State    *state,
                                                           Socket           *in_socket,
                                                           Buffer           *send,
                                                           WebSocket_Opcode  opcode)
{
  expect(state     != NULL);
  expect(in_socket != NULL);
  expect((send     != NULL) && (send->used > 0));

  Network_Return_Code return_code = network_ok;

  Arena                  *temp_arena   = get_temp_arena();
  u8                     *frame_start; frame_start  = temp_arena->start;
  WebSocket_Frame_Header *frame_header = push_struct(temp_arena, WebSocket_Frame_Header);

  frame_header->opcode = opcode;
  frame_header->mask   = 1;

  // TODO(antonio): network-order?
  u8 *frame_header_size = push_struct(temp_arena, u8);
  if (send->used < 126)
  {
    *frame_header_size    = (u8) send->used;
  }
  else if (send->used <= max_u16)
  {
    *frame_header_size    = 126;

    u16 *payload_16 = push_struct(temp_arena, u16);
    *payload_16     = (u16) send->used;
  }
  else
  {
    *frame_header_size    = 127;

    u64 *payload_64 = push_struct(temp_arena, u64);
    *payload_64     = (u16) send->used;
  }

  return(return_code);
}

#define TRADER_NETWORK_H
#endif
