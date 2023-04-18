#ifndef TRADER_NETWORK_H

enum
{
  network_ok,

  network_error_client_certificate_needed,

  network_error_send_failure,
  network_error_send_unknown,

  network_error_socket_disconnected,
  network_error_socket_error,

  network_error_unknown,

  network_return_code_count,
};
typedef i32 Network_Return_Code; 

external struct Socket;

external Socket nil_socket;

Network_Return_Code network_open(String_Const_utf8 host_name, u16 port, Socket *out_socket);
Network_Return_Code network_send(Socket *in_socket, Buffer to_send);

#define TRADER_NETWORK_H
#endif
