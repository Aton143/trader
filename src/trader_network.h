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

external struct Socket;
external Socket nil_socket;

internal void make_nil(Socket *check);
internal b32 is_nil(Socket *check);

internal Network_Return_Code network_startup(Network_State *out_state);
internal Network_Return_Code network_cleanup(Network_State *out_state);

internal Network_Return_Code network_connect(Network_State *state, Socket *out_socket, String_Const_utf8 host_name, u16 port);
internal Network_Return_Code network_disconnect(Network_State *state, Socket *in_out_socket);

internal Network_Return_Code network_send_simple(Network_State *state, Socket *in_socket, Buffer *to_send);
internal Network_Return_Code network_receive_simple(Network_State *state, Socket *in_socket, Buffer *receive);

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
  platform_print((char *) network_error_buffer);
}

#define TRADER_NETWORK_H
#endif
