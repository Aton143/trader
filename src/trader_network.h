#ifndef TRADER_NETWORK_H

struct Network_State
{
};

#define SSL_MAX_HOST_NAME_LENGTH 255
#define SSL_OK                   1
#define SSL_ERROR                0

enum
{
  network_ok,

  network_no_data,

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
  network_error_unimplemented,

  network_return_code_count,
};
typedef i32 Network_Return_Code; 

typedef u8 WebSocket_Opcode;
enum 
{
  websocket_opcode_continuation = 0x0,
  websocket_opcode_text,       // 0x1
  websocket_opcode_binary,     // 0x2
  websocket_opcode_reserved_3, // 0x3
  websocket_opcode_reserved_4, // 0x4
  websocket_opcode_reserved_5, // 0x5
  websocket_opcode_reserved_6, // 0x6
  websocket_opcode_reserved_7, // 0x7
  websocket_opcode_close,      // 0x8
  websocket_opcode_ping,       // 0x9
  websocket_opcode_pong,       // 0xa
  websocket_opcode_reserved_b, // 0xb
  websocket_opcode_reserved_c, // 0xc
  websocket_opcode_reserved_d, // 0xd
  websocket_opcode_reserved_e, // 0xe
  websocket_opcode_reserved_f, // 0xf

  websocket_opcode_count,
};

struct WebSocket_Frame_Header
{
  WebSocket_Opcode opcode: 4;
  u8               rsv:    3;
  u8               fin:    1;

  u8 length: 7;
  u8 mask:   1;
};

external struct Socket;
external Socket nil_socket;

external struct Asynchronous_Socket;
external Asynchronous_Socket nil_async_socket;

internal void make_nil(Socket *check);
internal b32 is_nil(Socket *check);

internal Network_Return_Code network_startup(Network_State *out_state);
internal Network_Return_Code network_cleanup(Network_State *out_state);

internal Network_Return_Code network_connect(Network_State     *state,
                                             Socket            *out_socket,
                                             String_Const_utf8  host_name,
                                             u16                port);

internal Network_Return_Code network_disconnect(Network_State *state, Socket *in_out_socket);

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *send);
internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive);

internal Network_Return_Code network_websocket_send_simple(Thread_Context   *thread_context,
                                                           Network_State    *state,
                                                           Socket           *in_socket,
                                                           Buffer           *send,
                                                           WebSocket_Opcode  opcode);

internal Network_Return_Code network_websocket_receive_simple(Network_State          *state,
                                                              Socket                 *in_socket,
                                                              Buffer                 *receive,
                                                              WebSocket_Frame_Header *header);

internal Network_Return_Code network_do_websocket_handshake(Network_State *state,
                                                            Socket *in_out_socket,
                                                            String_Const_utf8 host_name,
                                                            String_Const_utf8 query_path);

internal void network_print_error();

#define TRADER_NETWORK_H
#endif
