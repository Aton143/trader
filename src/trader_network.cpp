#include "trader_network.h"

internal void network_print_error()
{
  local_persist u8 network_error_buffer[512] = {};
  zero_literal(network_error_buffer);

  expect(!"removed OpenSSL dependency...");

  platform_debug_print((char *) "\n");
  platform_debug_print((char *) network_error_buffer);
  platform_debug_print((char *) "\n");
}

internal Network_Return_Code network_websocket_send_simple(Thread_Context   *thread_context,
                                                           Network_State    *state,
                                                           Socket           *in_socket,
                                                           Buffer           *send,
                                                           WebSocket_Opcode  opcode)
{
  expect(state     != NULL);
  expect(in_socket != NULL);
  expect((send     != NULL) && (send->used > 0));

  Network_Return_Code return_code = network_ok;

  Arena *temp_arena            = get_temp_arena(thread_context);
  u8    *frame_start           = temp_arena->start;
  u64    temp_arena_start_used = temp_arena->used;

  WebSocket_Frame_Header *frame_header = push_struct(temp_arena, WebSocket_Frame_Header);
  *frame_header = {};

  frame_header->opcode = opcode;
  frame_header->mask   = 1;
  frame_header->fin    = 1;

  if (send->used < 126)
  {
    frame_header->length = (u8) send->used;
  }
  else if (send->used <= max_u16)
  {
    frame_header->length = 126;

    // NOTE(antonio): network-order
    u16 *payload_16 = push_struct(temp_arena, u16);
    *payload_16     = byte_swap_16((u16) send->used);
  }
  else
  {
    frame_header->length = 127;

    u64 *payload_64 = push_struct(temp_arena, u64);
    // NOTE(antonio): network-order
    *payload_64     = byte_swap_64(send->used);
  }

  // TODO(antonio): network-order?
  // *((u16 *) frame_header) = byte_swap_16(*((u16 *) frame_header));

  u8 *frame_masking = push_array(temp_arena, u8, 4);
  frame_masking[0] = 0x12;
  frame_masking[1] = 0x34;
  frame_masking[2] = 0x56;
  frame_masking[3] = 0x78;

  // TODO(antonio): not sure about the copy
  u8 *payload = push_array(temp_arena, u8, send->used);
  for (u64 payload_index = 0;
       payload_index     < send->used;
       ++payload_index)
  {
    *payload++ = send->data[payload_index] ^ frame_masking[payload_index & 0x3];
  }

  Buffer frame_and_payload;

  frame_and_payload.data = frame_start;
  frame_and_payload.used = frame_and_payload.size = (u64) (temp_arena->used - temp_arena_start_used);

  return_code = network_send_simple(state, in_socket, &frame_and_payload);

  return(return_code);
}

internal Network_Return_Code network_websocket_receive_simple(Thread_Context         *thread_context,
                                                              Network_State          *state,
                                                              Socket                 *in_socket,
                                                              Buffer                 *receive,
                                                              WebSocket_Frame_Header *out_header)
{
  expect(state     != NULL);
  expect(in_socket != NULL);
  expect((receive  != NULL) && (receive->size > 0));

  Arena *temp_arena = get_temp_arena(thread_context);
  Buffer temp_buffer = push_buffer(temp_arena, 65536);
  zero_memory_block(temp_buffer.data, 65536);

  WebSocket_Frame_Header header = {};
  Network_Return_Code return_code = network_receive_simple(state, in_socket, &temp_buffer);

  if (return_code == network_ok)
  {
    expect_message(return_code == network_ok, "expected ok but this may change in the future");
    expect_message(temp_buffer.used >= 2, "expected at least two bytes for the header");

    u8 *receive_position = temp_buffer.data;
    u32 receive_index    = 0;

    header = *((WebSocket_Frame_Header *) receive_position);

    receive_position += sizeof(header);
    receive_index    += sizeof(header);

    u64 payload_size    = header.length;
    u32 payload_advance = 0;

    // NOTE(antonio): network-order
    if (payload_size == 126)
    {
      u16 *p16 = (u16 *) receive_position;
      payload_size = byte_swap_16(*p16);

      payload_advance = sizeof(*p16);
    }
    else if (payload_size == 127)
    {
      u64 *p64 = (u64 *) receive_position;
      payload_size = byte_swap_64(*p64);

      payload_advance = sizeof(*p64);
    }

    receive_position += payload_advance;
    receive_index    += payload_advance;

    // NOTE(antonio): per the spec (msb must be 0)
    expect((bit_64 & payload_size) == 0);

    u8 masking_key[4] = {};
    if (header.mask)
    {
      u32 *frame_masking_key = (u32 *) receive_position;

      // TODO(antonio): verify
      u32 *masking_key_conv = (u32 *) masking_key;
      *masking_key_conv = byte_swap_32(*frame_masking_key);

      receive_position += sizeof(*frame_masking_key);
      receive_index    += sizeof(*frame_masking_key);
    }

    // NOTE(antonio): we have the requisites to get the payload!
    for (u64 payload_index = 0;
         payload_index < payload_size;
         ++payload_index)
    {
      receive->data[payload_index] = *receive_position ^ masking_key[payload_index & 0x3];
      receive_position++;
    }
  }

  if (out_header)
  {
    *out_header = header;
  }

  return(return_code);
}
