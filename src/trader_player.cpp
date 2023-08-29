#include "trader_player.h"
#include "trader_ui.h"

internal inline Player_Context *player_get_context(void)
{
  return(&global_player_context);
}

internal void player_add_input(Key_Event event, b32 is_key_down)
{
  Player_Context *player_context = player_get_context();

  Player_Input_Event input = {};

  switch (event)
  {
    case key_event_a:
    case key_event_left_arrow:
    {
      input.event = player_input_clockwise;
    } break;

    case key_event_d:
    case key_event_right_arrow:
    {
      input.event = player_input_counter_clockwise;
    } break;
  }

  if (input.event != player_input_none)
  {
    input.flags |= (!!is_key_down) << 1;
    ring_buffer_append(&player_context->input_queue, &input, sizeof(input));
  }
}

internal void player_apply_input(void)
{
  Player_Context *player_context = player_get_context();
  Player_Input_Event input;

  f32 to = 0.0f;
  f32 is_down = 0.0f;

  while (player_context->input_queue.read != player_context->input_queue.write) 
  {
    ring_buffer_pop_and_put_struct(&player_context->input_queue, &input);

    is_down = (input.flags >> 1) & 1 ? 1.0f : 0.0f;

    switch (input.event)
    {
      case player_input_clockwise:
      {
        to += -player_context->rotation_max_speed * is_down;
      } break;

      case player_input_counter_clockwise:
      {
        to += player_context->rotation_max_speed * is_down;
      } break;
    }
  }

  player_context->rotation_speed = lerpf(player_context->rotation_speed, player_context->lerp_factor, to);
  player_context->rotation += player_context->rotation_speed;
}
