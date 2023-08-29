#include "trader_player.h"
#include "trader_ui.h"

internal inline Player_Context *player_get_context(void)
{
  return(&global_player_context);
}

internal void player_add_input(Key_Event event, b32 is_key_down)
{
  Player_Context *player_context = player_get_context();

  if (is_key_down)
  {
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

    if (input != player_input_none)
    {
      ring_buffer_append(&player_context->input_queue, &input, sizeof(input));
    }
  }
}

internal void player_apply_input(void)
{
  Player_Context *player_context = player_get_context();
  Player_Input input;

  while (player_context->input_queue.read != player_context->input_queue.write) 
  {
    ring_buffer_pop_and_put_struct(&player_context->input_queue, &input);

    switch (input)
    {
      case player_input_clockwise:
      {
        player_context->rotation -= player_context->rotation_delta;
      } break;

      case player_input_counter_clockwise:
      {
        player_context->rotation += player_context->rotation_delta;
      } break;
    }
  }
}
