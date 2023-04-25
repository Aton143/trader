#ifndef TRADER_NETWORK_H

struct Network_State
{
  SSL_CTX *ssl_context;
};

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

  network_error_unknown,

  network_return_code_count,
};
typedef i32 Network_Return_Code; 

external struct Socket;
external Socket nil_socket;

internal void make_nil(Socket *check);
internal b32 is_nil(Socket *check);

internal Network_Return_Code network_startup(Network_State *out_state);

internal Network_Return_Code network_connect(Network_State *state, String_Const_utf8 host_name, u16 port, Socket *out_socket);
internal Network_Return_Code network_disconnect(Network_State *state, Socket *in_out_socket);

internal Network_Return_Code network_send(Network_State *state, Socket *in_socket, Buffer to_send);
internal Network_Return_Code network_receive(Network_State *state, Socket *in_socket, Buffer *in_receive);

internal Network_Return_Code network_cleanup(Network_State *out_state);

#define TRADER_NETWORK_H
#endif
