#include "trader_player.h"
#include "trader_ui.h"

internal inline Player_Context *player_get_context(void)
{
  return(&global_player_context);
}

global char *last_key;

internal void player_add_input(Key_Event event, b32 is_key_down)
{
  Player_Context *player_context = player_get_context();

  Player_Input input = player_input_none;

  switch (event)
  {
    case key_event_a:
    case key_event_left_arrow:
    {
      input = player_input_clockwise;
    } break;

    case key_event_d:
    case key_event_right_arrow:
    {
      input = player_input_counter_clockwise;
    } break;
  }

  player_context->inputs[input] = is_key_down ? 1.0f : 0.0f;
}

internal void player_apply_input(void)
{
  Player_Context *player_context = player_get_context();

  f32 to = 0.0f;

  to += -player_context->rotation_max_speed * player_context->inputs[player_input_clockwise];
  to +=  player_context->rotation_max_speed * player_context->inputs[player_input_counter_clockwise];

  player_context->rotation_speed = lerpf(player_context->rotation_speed, player_context->lerp_factor, to);
  player_context->rotation += player_context->rotation_speed;
}
