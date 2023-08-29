#ifndef TRADER_PLAYER_H

typedef u32 Player_Input;
enum
{
  player_input_none,
  player_input_clockwise,
  player_input_counter_clockwise,
};

struct Player_Context
{
  Ring_Buffer input_queue;
  f32 rotation_delta;
  f32 rotation;
};

global u8 __input_queue_buffer[128];
global Player_Context global_player_context;

internal inline Player_Context *player_get_context(void);
internal void player_add_input(Key_Event event, b32 is_key_down);
internal void player_apply_input(void);

#define TRADER_PLAYER_H
#endif
