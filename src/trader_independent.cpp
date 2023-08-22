#include "trader_independent.h"

void update_and_render(Game_Data *game_data)
{
  UI_Context            *ui            = ui_get_context();
  Common_Render_Context *common_render = render_get_common_context();

  Rect_f32 render_rect = render_get_client_rect();

  ui_initialize_frame();

  i32 panel_float_index = 0;
  ui_push_background_color(rgba_from_u8(255, 255, 255, 255));
  ui_make_panel(axis_split_vertical,
                &panel_floats[panel_float_index++],
                string_literal_init_type("first", utf8));

  ui_push_text_color(1.0f, 1.0f, 1.0f, 1.0f);

  ui_do_text_edit(&debug_teb, "text editor");
  ui_do_formatted_string("Last frame time: %2.6fs", game_data->last_frame_time);
  ui_do_formatted_string("Mouse position: (%.0f, %.0f)",
                         (double) ui->mouse_pos.x, (double) ui->mouse_pos.y);

  if (ui->mouse_area == mouse_area_in_client)
  {
    ui_do_string(string_literal_init_type("Mouse is in client", utf8));
  }
  else if (ui->mouse_area == mouse_area_out_client)
  {
    ui_do_string(string_literal_init_type("Mouse is not in client", utf8));
  }
  else
  {
    ui_do_string(string_literal_init_type("I don't where the hell the mouse is", utf8));
  }

  ui_do_formatted_string("Slider float: %.4f", (double) slider_float);
  ui_do_slider_f32(string_literal_init_type("slider", utf8), &slider_float, 0.0f, 1.0f);

  ui_do_formatted_string("Interaction Results:");
  for (u32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index)
  {
    UI_Interaction *cur_interaction = &ui->interactions[interaction_index];
    ui_do_formatted_string("Key: %d, Value: %d, Frames Left: %d",
                           (i32) cur_interaction->key,
                           (i32) cur_interaction->event,
                           (i32) cur_interaction->frames_left);
  }

  ui_do_formatted_string("Active key: %d", (i32) ui->active_key);
  ui_do_formatted_string("Hot Key: %d", (i32) ui->hot_key);

  ui_prepare_render_from_panels(ui_get_sentinel_panel(), render_rect);
  ui_flatten_draw_layers();
}
