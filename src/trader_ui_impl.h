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
    Arena *widget_stack = temp_arena;
    append_struct(widget_stack, &ui->allocated_widgets);

    // NOTE(antonio): THIS HAS TO BE A STACK OF POINTERS
    Widget *cur_widget = NULL;
    while ((cur_widget = (Widget *) _arena_get_top(widget_stack, sizeof(Widget *))))
    {
      Widget *parent = cur_widget->parent;

      if (cur_widget->size_flags & size_flag_copy_parent_size_x)
      {
        cur_widget->rectangle.x0 = parent->rectangle.x0;
        cur_widget->rectangle.x1 = parent->rectangle.x1;
      }

      if (cur_widget->size_flags & size_flag_copy_parent_size_y)
      {
        cur_widget->rectangle.y0 = parent->rectangle.y0;
        cur_widget->rectangle.y1 = parent->rectangle.y1;
      }

      if (cur_widget->size_flags & size_flag_fill_rest_of_axis_x)
      {
        cur_widget->size_flags |= size_flag_to_be_sized_x;
      }

      if (cur_widget->size_flags & size_flag_fill_rest_of_axis_y)
      {
        cur_widget->size_flags |= size_flag_to_be_sized_y;
      }

      // NOTE(antonio): this needs to be communicated to the parent
      if (cur_widget->computed_size_in_pixels.x > 0.0f)
      {
        if (parent && (parent->size_flags & size_flag_content_size_x))
        {
          parent->computed_size_in_pixels.x += cur_widget->computed_size_in_pixels.x;
        }
      }

      // NOTE(antonio): this needs to be communicated to the parent
      if (cur_widget->computed_size_in_pixels.y > 0.0f)
      {
        if (parent && (parent->size_flags & size_flag_content_size_y))
        {
          parent->computed_size_in_pixels.y += cur_widget->computed_size_in_pixels.y;
        }
      }

      Widget *first_child = NULL;
      for (Widget *cur_child = cur_widget->first_child;
           cur_child != first_child;
           cur_child = cur_child->next_sibling)
      {
        arena_append(widget_stack, &cur_child, sizeof(Widget *));
        first_child = cur_widget->first_child;
      }
    }
  }

  arena_reset(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(temp_arena->size, Widget *));

    Widget *first_child = NULL;
    for (Widget *cur_child = ui->allocated_widgets->first_child;
         cur_child != first_child;
         cur_child = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = ui->allocated_widgets->first_child;
    }

    while (widget_queue.write != widget_queue.read)
    {
      Widget *cur_widget = NULL;
      ring_buffer_pop_and_put(&widget_queue, cur_widget, sizeof(cur_widget));

      f32 remaining_width = rect_get_width(&cur_widget->rectangle);
      f32 max_height = 0.0f;

      u32 to_be_sized_x = 0;

      // NOTE(antonio): if content_size & has children,
      // then need to know complete children sizes for that dimension
      // TODO(antonio): check this out for ^
      first_child = NULL;
      for (Widget *cur_child = cur_widget->first_child;
           cur_child != first_child;
           cur_child = cur_child->next_sibling)
      {
        if (cur_child->size_flags & size_flag_content_size_x)
        {
          remaining_width -= cur_child->computed_size_in_pixels.x;
          if (remaining_width < 0.0f)
          {
            remaining_width = 0.0f;
          }
        }

        if (cur_child->size_flags & size_flag_content_size_y)
        {
          max_height = max(max_height, cur_child->computed_size_in_pixels.y);
        }

        if (cur_child->size_flags & size_flag_to_be_sized_x)
        {
          to_be_sized_x++;
        }

        first_child = cur_widget->first_child;
      }

      // NOTE(antonio): already know the size
      {
        V2_f32 cur_top_left = {cur_widget->rectangle.x0, cur_widget->rectangle.y0};

        first_child = NULL;
        for (Widget *cur_child = cur_widget->first_child;
             cur_child != first_child;
             cur_child = cur_child->next_sibling)
        {
          if (cur_child->size_flags & size_flag_to_be_sized_x)
          {
            assert(to_be_sized_x > 0);
            cur_child->computed_size_in_pixels.x = (remaining_width / ((f32) to_be_sized_x));
          }

          cur_child->computed_size_in_pixels.y = max_height;

          {
            cur_child->rectangle.x0 = cur_top_left.x;
            cur_child->rectangle.y0 = cur_top_left.y;

            // TODO(antonio): this could mess up text alignment
            cur_child->rectangle.x1 = cur_top_left.x + cur_child->computed_size_in_pixels.x;
            cur_child->rectangle.x1 = cur_top_left.y + cur_child->computed_size_in_pixels.y;
          }

          cur_top_left.x += cur_child->computed_size_in_pixels.x;

          first_child = cur_widget->first_child;

          if (cur_child->first_child)
          {
            ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
          }
        }
      }
    }
  }
}

#define TRADER_UI_IMPL_H
#endif
