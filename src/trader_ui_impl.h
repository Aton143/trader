#ifndef TRADER_UI_IMPL_H

internal void ui_initialize_frame(void)
{
  UI_Context *ui = ui_get_context();

  zero_memory_block(ui->widget_memory, sizeof(Widget) * ui->current_widget_count);

  Widget *current_free;
  for (u32 widget_index = 0;
       widget_index < ui->current_widget_count;
       ++widget_index)
  {
    current_free = &ui->widget_memory[widget_index];
    current_free->next_sibling = &ui->widget_memory[widget_index + 1];
  }

  Widget *sentinel_widget    = ui->widget_memory;

  sentinel_widget->rectangle = render_get_client_rect();
  sentinel_widget->string    = string_literal_init_type("sentinel", utf8);

  ui->text_gutter_dim       = default_text_gutter_dim;
  ui->text_color            = default_text_color;
  ui->background_color      = default_background_color;
  ui->max_widget_count      = (u32) (ui->widget_memory_size / sizeof(Widget));
  ui->current_widget_count  = 1;

  ui->widget_free_list_head = ui->widget_memory + 1;
  ui->allocated_widgets     = sentinel_widget;
  ui->current_parent        = sentinel_widget;

  ui->drag_delta            = {0, 0};
}

internal void ui_set_text_color(f32 r, f32 g, f32 b, f32 a)
{
  UI_Context *ui = ui_get_context();
  ui->text_color = rgba(r, g, b, a);
}

internal void ui_set_background_color(f32 r, f32 g, f32 b, f32 a)
{
  UI_Context *ui = ui_get_context();
  ui->background_color = rgba(r, g, b, a);
}

internal void ui_push_parent(Widget *widget)
{
  UI_Context *ui = ui_get_context();
  ui->current_parent = widget;
}

internal void ui_pop_parent(void)
{
  UI_Context *ui = ui_get_context();
  if (ui->current_parent->parent)
  {
    ui->current_parent = ui->current_parent->parent;
  }
}

internal UI_Key ui_make_key(String_Const_utf8 string)
{
  UI_Key key = hash(string.str, string.size);
  return(key);
}

internal b32 ui_is_key_equal(UI_Key a, UI_Key b)
{
  b32 result = (a == b);
  return(result);
}

#define TRADER_UI_IMPL_H
#endif
