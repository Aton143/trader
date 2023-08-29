#ifndef TRADER_PLAYER_H

typedef u32 Player_Input;
enum
{
  player_input_none,
  player_input_clockwise,
  player_input_counter_clockwise,
};

struct Player_Control
{
  Ring_Buffer input_queue;
};

global u8 __input_queue_buffer[128];
global Player_Control global_control;

#define TRADER_PLAYER_H
#endif
