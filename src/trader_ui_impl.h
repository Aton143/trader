#if !defined(TRADER_UI_IMPL_H)

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

internal void ui_make_widget(Widget_Flag       widget_flags,
                             Widget_Size_Flag  size_flags,
                             String_Const_utf8 string)
{
  // TODO(antonio): sprint nation?
  UI_Context     *ui     = ui_get_context();
  Render_Context *render = render_get_context();

  unused(widget_flags);
  unused(string);
  unused(size_flags);

  if (ui->current_widget_count < ui->max_widget_count)
  {
    Widget *widget = ui->widget_free_list_head;

    ++ui->current_widget_count;
    ui->widget_free_list_head = ui->widget_free_list_head->next_sibling;

    zero_struct(widget);

    // TODO(antonio): append_widget?
    Widget *cur_par = ui->current_parent;

    if (cur_par->first_child == NULL)
    {
      widget->next_sibling     = widget;
      widget->previous_sibling = widget;

      cur_par->first_child = widget;
      cur_par->last_child  = widget;
    }
    else
    {
      widget->previous_sibling = cur_par->last_child;
      widget->next_sibling     = cur_par->first_child;

      cur_par->last_child->next_sibling      = widget;
      cur_par->first_child->previous_sibling = widget;
      cur_par->last_child                    = widget;
    }

    widget->parent = cur_par;

    f32 content_height = 0;
    f32 content_width = 0;

    if (widget_flags & widget_flag_draw_text)
    {
      // NOTE(antonio): calculate text width

      // NOTE(antonio): assuming that font height was found when pushed
      stbtt_packedchar *packed_char_start = render->atlas->char_data + render_get_packed_char_start(ui->text_height);
      f32 font_scale = stbtt_ScaleForPixelHeight(&render->atlas->font_info, ui->text_height);

      u64 string_index;
      for (string_index = 0;
           // NOTE(antonio): just in case they're being rat bastards
           (string.str[string_index] != '\0') && (string_index < string.size);
           ++string_index)
      {
        stbtt_packedchar *cur_packed_char = packed_char_start + (string.str[string_index] - starting_code_point);
        f32 cur_char_height = (f32) (cur_packed_char->yoff2 - cur_packed_char->yoff);
        f32 cur_char_width  = (f32) (cur_packed_char->xoff2 - cur_packed_char->xoff);

        content_height  =  max(content_height, cur_char_height);
        content_width  += (cur_char_width + cur_packed_char->xadvance);

        if (string_index < string.size - 1)
        {
          content_width += font_scale *
                           stbtt_GetCodepointKernAdvance(&render->atlas->font_info,
                                                         string.str[string_index],
                                                         string.str[string_index + 1]);
        }
      }

      widget->computed_size_in_pixels = {content_width, content_height};
    }

    widget->widget_flags  = widget_flags;
    widget->size_flags    = size_flags;
    widget->string        = string;
  }
}

internal void ui_prepare_render(void)
{
  Arena          *temp_arena = get_temp_arena();
  UI_Context     *ui         = ui_get_context();
  Render_Context *render     = render_get_context();

  unused(ui);
  unused(render);

  assert((render->render_data.used == 0) && "for now, assume that the render data is required to be empty");

  {
    // NOTE(antonio): stack grows from high to low
  //   Arena *widget_stack     = temp_arena;
    // arena_push_str(
  }

  arena_reset(temp_arena);
}

#define TRADER_UI_IMPL_H
#endif
