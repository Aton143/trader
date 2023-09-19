#ifndef TRADER_PLAYER_H

typedef u16 Player_Input;
enum
{
  player_input_none,

  player_input_clockwise,
  player_input_counter_clockwise,

  player_input_count,
};

struct Player_Context
{
  f32 inputs[player_input_count];

  f32 rotation_max_speed;
  f32 rotation_speed;

  f32 rotation;
  f32 lerp_factor;

  b32 dragging;
  u32 choose; // 0 -> no, 1 -> x, 2 -> y

  V2_f32 initial_mouse_pos;
  V2_f32 cur_mouse_pos;
  V2_f32 prev_mouse_pos;
};

global u8 __input_queue_buffer[128];
global Player_Context global_player_context;

internal inline Player_Context *player_get_context(void);
internal void player_add_input(Key_Event event, b32 is_key_down);
internal void player_apply_input(void);

#define TRADER_PLAYER_H
#endif
