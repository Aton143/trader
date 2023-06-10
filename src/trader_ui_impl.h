#if !defined(TRADER_UI_IMPL_H)

internal Widget *ui_get_sentinel(void)
{
  UI_Context *ui       = ui_get_context();
  Widget     *sentinel = ui->allocated_widgets;
  return(sentinel);
}

internal void ui_add_key_event(Key_Event event, b32 is_down)
{
  UI_Context *ui = ui_get_context();

  if (event == key_mod_event_control)
  {
    ui->mod_keys.control = (b8) is_down;
  }
  else if (event == key_mod_event_shift)
  {
    ui->mod_keys.shift = (b8) is_down;
  }
  else if (event == key_mod_event_alt)
  {
    ui->mod_keys.alt = (b8) is_down;
  }
  else if (event == key_mod_event_super)
  {
    ui->mod_keys.super = (b8) is_down;
  }

  ui->key_events[event] = (b8) is_down;
}


internal void ui_initialize_frame(void)
{
  UI_Context *ui = ui_get_context();

  arena_reset_zero(ui->string_pool);
  zero_memory_block(ui->widget_memory, sizeof(Widget) * ui->current_widget_count);

  Widget *current_free;
  for (u32 widget_index = 0;
       widget_index < ui->current_widget_count;
       ++widget_index)
  {
    current_free               = &ui->widget_memory[widget_index];
    current_free->next_sibling = &ui->widget_memory[widget_index + 1];
    current_free->string       = string_literal_init_type("you reset this one", utf8);
  }

  Widget *sentinel_widget    = ui->widget_memory;

  zero_struct(sentinel_widget);

  sentinel_widget->rectangle = render_get_client_rect();
  sentinel_widget->string    = string_literal_init_type("sentinel", utf8);

  ui->text_height            = default_text_height;
  ui->text_gutter_dim        = default_text_gutter_dim;
  ui->text_color             = default_text_color;

  copy_memory_block(ui->background_color, (void *) default_background_color, sizeof(default_background_color));

  ui->max_widget_count       = (u32) (ui->widget_memory_size / sizeof(Widget));
  ui->current_widget_count   = 1;

  ui->widget_free_list_head  = ui->widget_memory + 1;
  ui->allocated_widgets      = sentinel_widget;
  ui->current_parent         = sentinel_widget;

  default_persistent_data    = {};
}

internal void ui_update_persistent_data(Persistent_Widget_Data *data)
{
  UI_Context *ui  = ui_get_context();
  for (u32 pers_index = 0;
       pers_index < array_count(ui->persistent_data);
       ++pers_index)
  {
    Persistent_Widget_Data *cur_pers = &ui->persistent_data[pers_index];

    if (cur_pers->key == data->key)
    {
      break;
    }
    else if (cur_pers->key == nil_key)
    {
      copy_struct(cur_pers, data);
      break;
    }
  }
}

internal Persistent_Widget_Data *ui_search_persistent_data(Widget *widget)
{
  UI_Context             *ui     = ui_get_context();
  Persistent_Widget_Data *result = &default_persistent_data;

  copy_memory_block(&result->background_color,
                    (void *) widget->end_background_color,
                    sizeof(result->background_color));

  for (u32 pers_index = 0;
       pers_index < array_count(ui->persistent_data);
       ++pers_index)
  {
    Persistent_Widget_Data *cur_pers = &ui->persistent_data[pers_index];
    if (cur_pers->key == widget->key)
    {
      result = cur_pers;
      break;
    }
  }

  return(result);
}

internal void ui_set_text_height(f32 height)
{
  UI_Context *ui  = ui_get_context();
  ui->text_height = height;
}

internal void ui_push_text_color(f32 r, f32 g, f32 b, f32 a)
{
  expect(is_between_inclusive(0.0f, r, 1.0f) && 
         is_between_inclusive(0.0f, g, 1.0f) && 
         is_between_inclusive(0.0f, b, 1.0f) && 
         is_between_inclusive(0.0f, a, 1.0f));

  UI_Context *ui = ui_get_context();
  ui->text_color = rgba(r, g, b, a);
}

internal void ui_pop_text_color(void)
{
  UI_Context *ui = ui_get_context();
  ui->text_color = default_text_color;
}

internal void ui_push_background_color(f32 r, f32 g, f32 b, f32 a)
{
  expect(is_between_inclusive(0.0f, r, 1.0f) && 
         is_between_inclusive(0.0f, g, 1.0f) && 
         is_between_inclusive(0.0f, b, 1.0f) && 
         is_between_inclusive(0.0f, a, 1.0f));

  UI_Context *ui = ui_get_context();

  ui->background_color[0] = rgba(r, g, b, a);
  ui->background_color[1] = rgba(r, g, b, a);
  ui->background_color[2] = rgba(r, g, b, a);
  ui->background_color[3] = rgba(r, g, b, a);
}

internal void ui_pop_background_color()
{
  UI_Context *ui = ui_get_context();
  copy_memory_block(ui->background_color, (void *) default_background_color, sizeof(default_background_color));
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

// TODO(antonio): consider what to do for format and string
internal void ui_do_string(String_Const_utf8 string)
{
  UI_Context *ui = ui_get_context();
  Widget *last_parent = ui->current_parent;

  ui_make_widget(widget_flag_draw_background,
                 size_flag_text_content,
                 string_literal_init_type("String parent", utf8));

  Widget *text_parent = ui->current_parent->last_child;
  ui_push_parent(text_parent);

  String_Const_utf8 copy_string;

  // NOTE(antonio): string pool gets cleared out every frame
  copy_string.str = (utf8 *) arena_push(ui->string_pool, string.size + 1);
  copy_memory_block(copy_string.str, string.str, string.size);
  copy_string.size = string.size + 1;

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content,
                 copy_string);

  ui_pop_parent();
  ui_push_parent(last_parent);
}

internal void ui_do_formatted_string(char *format, ...)
{
  UI_Context *ui          = ui_get_context();
  Widget     *last_parent = ui->current_parent;

  ui_make_widget(widget_flag_draw_background,
                 size_flag_text_content,
                 string_literal_init_type("Text parent", utf8));

  Widget *text_parent = ui->current_parent->last_child;
  ui_push_parent(text_parent);

  va_list args;
  va_start(args, format);

  // NOTE(antonio): string pool gets cleared out every frame
  // NOTE(antonio): speculative "sprintf'ing"
  String_Const_utf8 sprinted_text;

  char *string_start = (char *) (ui->string_pool->start + ui->string_pool->used);
  sprinted_text.size = stbsp_vsnprintf(string_start, 512, format, args);
  sprinted_text.str  = (utf8 *) string_start;

  arena_push(ui->string_pool, sprinted_text.size + 1);
  va_end(args);

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content,
                 sprinted_text);

  ui_pop_parent();
  ui_push_parent(last_parent);
}

internal b32 ui_do_button(String_Const_utf8 string)
{
  UI_Context *ui          = ui_get_context();
  Widget     *last_parent = ui->current_parent;
  b32         result      = false;

  String_Const_utf8 button_parent_to_hash_prefix = string_literal_init_type("Button parent::", utf8);
  String_Const_utf8 button_parent_to_hash = concat_string_to_c_string(ui->string_pool, button_parent_to_hash_prefix, string);

  ui_make_widget(widget_flag_draw_background | widget_flag_clickable,
                 size_flag_text_content,
                 button_parent_to_hash);

  Widget *button_text_parent = ui->current_parent->last_child;
  ui_push_parent(button_text_parent);

  String_Const_utf8 copy_string;

  copy_string.str  = (utf8 *) arena_push(ui->string_pool, string.size + 1);
  copy_memory_block(copy_string.str, string.str, string.size);
  copy_string.size = string.size + 1;

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content,
                 copy_string);

  ui_pop_parent();
  ui_push_parent(last_parent);

  for (u32 interaction_index = 0;
       interaction_index < ui->interaction_index;
       ++interaction_index) 
  {
    if (ui->interactions[interaction_index].key == button_text_parent->key)
    {
      result = true;
    }
  }

  button_text_parent->end_background_color[0] = rgba(0.0f, 0.0f, 0.0f, 1.0f);
  button_text_parent->end_background_color[1] = rgba(1.0f, 1.0f, 1.0f, 1.0f);
  button_text_parent->end_background_color[2] = rgba(0.0f, 0.0f, 0.0f, 1.0f);
  button_text_parent->end_background_color[3] = rgba(1.0f, 1.0f, 1.0f, 1.0f);

  return(result);
}

internal void ui_do_slider_f32(String_Const_utf8 string, f32 *in_out_value, f32 minimum, f32 maximum)
{
  UI_Context *ui          = ui_get_context();
  Widget     *last_parent = ui->current_parent;

  expect(in_out_value != NULL);
  expect(minimum <= maximum);

  String_Const_utf8 slider_parent_to_hash_prefix = string_literal_init_type("slider parent::", utf8);
  String_Const_utf8 slider_parent_to_hash = concat_string_to_c_string(ui->string_pool, slider_parent_to_hash_prefix, string);

  unused(ui);
  unused(last_parent);
  unused(slider_parent_to_hash);
  unused(in_out_value);
  unused(minimum);
  unused(maximum);

  ui_make_widget(widget_flag_draw_background | widget_flag_clickable,
                 size_flag_content_size_x | size_flag_content_size_y,
                 slider_parent_to_hash);
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

    f32 content_width = (f32) ui->text_gutter_dim.x;

    if (widget_flags & widget_flag_draw_text)
    {
      // NOTE(antonio): calculate text width

      // NOTE(antonio): assuming that font height was found when pushed
      stbtt_packedchar *packed_char_start = render->atlas->char_data + render_get_packed_char_start(ui->text_height);
      // f32 font_scale = stbtt_ScaleForPixelHeight(&render->atlas->font_info, ui->text_height);

      u64 string_index;
      for (string_index = 0;
           // NOTE(antonio): just in case they're being rat bastards
           (string.str[string_index] != '\0') && (string_index < string.size);
           ++string_index)
      {
        // TODO(antonio): deal with new lines more gracefully
        if (is_newline(string.str[string_index])) 
        {
          continue;
        }
        else
        {
          stbtt_packedchar *cur_packed_char = packed_char_start + (string.str[string_index] - starting_code_point);
          // f32 cur_char_height = (f32) (cur_packed_char->yoff2 - cur_packed_char->yoff);
          // f32 cur_char_width  = (f32) (cur_packed_char->xoff2 - cur_packed_char->xoff);

          content_width  += cur_packed_char->xadvance;

          /*
          if (string_index < string.size - 1)
          {
            content_width += font_scale *
              stbtt_GetCodepointKernAdvance(&render->atlas->font_info,
                                            string.str[string_index],
                                            string.str[string_index + 1]);
          }
          */
        }
      }

      widget->computed_size_in_pixels =
      {
        content_width   + (f32) (2 * ui->text_gutter_dim.x),
        ui->text_height // + (f32) (/*2 * */ui->text_gutter_dim.y)
      };
    }

    widget->widget_flags     = widget_flags;
    widget->size_flags       = size_flags;
    widget->string           = string;
    widget->text_color       = ui->text_color;
    widget->key              = ui_make_key(string);

    copy_memory_block(widget->end_background_color, ui->background_color, sizeof(ui->background_color));
  }
}

internal void ui_prepare_render(void)
{
  Global_Platform_State *global_state = platform_get_global_state();
  Arena                 *temp_arena   = get_temp_arena();
  UI_Context            *ui           = ui_get_context();
  Render_Context        *render       = render_get_context();

  u64 ring_buffer_size       = temp_arena->size / 2;

  expect_message(render->render_data.used == 0, "expected no render data");
  expect_message(compare_string_utf8(ui->allocated_widgets->string, string_literal_init_type("sentinel", utf8)),
                 "expected first widget to be sentinel widget");
  {
    // NOTE(antonio): stack grows from high to low
    Arena *widget_stack = temp_arena;

    Widget **data = &ui->allocated_widgets;
    arena_append(widget_stack, data, sizeof(Widget *));

    // NOTE(antonio): first-child biased loop
    Widget **stack_top = NULL;
    Widget *cur_widget = NULL;
    while ((stack_top = arena_get_top(widget_stack, Widget *)))
    {
      cur_widget = *stack_top;
      arena_pop(widget_stack, sizeof(Widget **));

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
          parent->computed_size_in_pixels.y = max(parent->computed_size_in_pixels.y,
                                                  cur_widget->computed_size_in_pixels.y);
        }
      }

      Widget *first_child = NULL;
      for (Widget *cur_child  = cur_widget->first_child;
           cur_child         != first_child;
           cur_child          = cur_child->next_sibling)
      {
        arena_append(widget_stack, &cur_child, sizeof(Widget *));
        first_child = cur_widget->first_child;
      }
    }
  }

  arena_reset_zero(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(ring_buffer_size, Widget **));

    // NOTE(antonio): don't care about the sentinel
    Widget *first_child = NULL;
    for (Widget *cur_child  = ui->allocated_widgets->first_child;
         cur_child         != first_child;
         cur_child          = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = ui->allocated_widgets->first_child;
    }

    V2_f32 cur_top_left = rect_get_top_left(&ui_get_sentinel()->rectangle);

    // NOTE(antonio): resolve "implicit" sizes with current information, level-order 
    while (widget_queue.write != widget_queue.read)
    {
      Widget *cur_widget = NULL;
      ring_buffer_pop_and_put_struct(&widget_queue, &cur_widget);

      V2_f32 pre_sizing_top_left = cur_top_left;

      f32 remaining_width = rect_get_width(&cur_widget->rectangle);
      f32 max_height = 0.0f;

      u32 to_be_sized_x = 0;

      // NOTE(antonio): if content_size & has children,
      // then need to know complete children sizes for that dimension
      // TODO(antonio): check this out for ^
      first_child = NULL;
      for (Widget *cur_child  = cur_widget->first_child;
           cur_child         != first_child;
           cur_child          = cur_child->next_sibling)
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

      // NOTE(antonio): already know the size for those "to be sized"
      {
        first_child = NULL;
        for (Widget *cur_child  = cur_widget->first_child;
             cur_child         != first_child;
             cur_child          = cur_child->next_sibling)
        {
          if (cur_child->size_flags & size_flag_to_be_sized_x)
          {
            expect(to_be_sized_x > 0);
            cur_child->computed_size_in_pixels.x = (remaining_width / ((f32) to_be_sized_x));
          }

          cur_child->computed_size_in_pixels.y = max_height;

          {
            cur_child->rectangle.x0 = cur_top_left.x;
            cur_child->rectangle.y0 = cur_top_left.y;

            // TODO(antonio): this could mess up text alignment
            cur_child->rectangle.x1 = cur_top_left.x + cur_child->computed_size_in_pixels.x;
            cur_child->rectangle.y1 = cur_top_left.y + cur_child->computed_size_in_pixels.y;
          }

          cur_top_left.x += cur_child->computed_size_in_pixels.x;

          first_child = cur_widget->first_child;

          if (cur_child->first_child)
          {
            ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
          }
        }
      }

      cur_widget->rectangle.x0 = pre_sizing_top_left.x;
      cur_widget->rectangle.y0 = pre_sizing_top_left.y;

      cur_widget->rectangle.x1 = pre_sizing_top_left.x + cur_widget->computed_size_in_pixels.x;
      cur_widget->rectangle.y1 = pre_sizing_top_left.y + cur_widget->computed_size_in_pixels.y;

      cur_top_left.x  = 0.0f;
      cur_top_left.y += cur_widget->computed_size_in_pixels.y;
    }
  }

  arena_reset_zero(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(ring_buffer_size, Widget *));
    Widget *first_child = NULL;
    for (Widget *cur_child = ui->allocated_widgets->first_child;
         cur_child != first_child;
         cur_child = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = ui->allocated_widgets->first_child;
    }

    b32 hot_key_should_be_kept = false;

    // NOTE(antonio): create draw calls in parent->child level traversal
    while (widget_queue.read != widget_queue.write)
    {
      Widget *cur_widget = NULL;
      ring_buffer_pop_and_put(&widget_queue, &cur_widget, sizeof(Widget **));

      if (cur_widget->widget_flags & widget_flag_clickable)
      {
        RGBA_f32 saved_background_color[4];
        copy_memory_block((void *) saved_background_color,
                          (void *) cur_widget->end_background_color,
                          sizeof(saved_background_color));

        b32 mouse_left_change = ((ui->prev_frame_mouse_event & mouse_event_lclick) !=
                                 (ui->cur_frame_mouse_event  & mouse_event_lclick));

        if (ui_is_key_equal(ui->active_key, cur_widget->key))
        {
          b32 mouse_left_went_up = mouse_left_change && ((ui->cur_frame_mouse_event & mouse_event_lclick) == 0);
          if (mouse_left_went_up)
          {
            if (ui_is_key_equal(ui->hot_key, cur_widget->key))
            {
              b32 interaction_already_recorded = false;
              for (u32 interaction_index = 0;
                   interaction_index < array_count(ui->interactions);
                   ++interaction_index) 
              {
                UI_Interaction *cur_interaction = &ui->interactions[interaction_index];
                if (cur_interaction->key == cur_widget->key)
                {
                  interaction_already_recorded = true;
                  break;
                }
              }

              if (!interaction_already_recorded)
              {
                ui->interactions[ui->interaction_index++] = {cur_widget->key, 0, 2};
              }
            }

            ui->active_key = nil_key;
          }
        }
        else if (ui_is_key_equal(ui->hot_key, cur_widget->key))
        {
          b32 mouse_left_went_down = mouse_left_change && (ui->cur_frame_mouse_event & mouse_event_lclick);
          if (mouse_left_went_down)
          {
            ui->active_key = cur_widget->key;

            Persistent_Widget_Data pers_data = {cur_widget->key};
            copy_memory_block(&pers_data.background_color,
                              (void *) saved_background_color,
                              sizeof(pers_data.background_color));

            ui_update_persistent_data(&pers_data);
          }
        }

        // NOTE(antonio): is mouse over the widget? 
        V2_f32 mouse_pos = ui->mouse_pos;
        if (is_between_inclusive(cur_widget->rectangle.x0, mouse_pos.x, cur_widget->rectangle.x1) && 
            is_between_inclusive(cur_widget->rectangle.y0, mouse_pos.y, cur_widget->rectangle.y1))
        {
          if (ui->active_key == nil_key)
          {
            ui->hot_key = cur_widget->key;
          }

          hot_key_should_be_kept = true;
        }

        if ((cur_widget->key == ui->active_key) && (cur_widget->key == ui->hot_key))
        {
          cur_widget->end_background_color[0] = cur_widget->end_background_color[1];
          cur_widget->end_background_color[2] = cur_widget->end_background_color[3];

          cur_widget->end_background_color[1] = saved_background_color[0];
          cur_widget->end_background_color[3] = saved_background_color[2];
        }
        /*
        else
        {
          Persistent_Widget_Data pers_data = {cur_widget->key};
          copy_memory_block(&pers_data.background_color,
                            (void *) saved_background_color,
                            sizeof(pers_data.background_color));

          ui_update_persistent_data(&pers_data);
        }
        */
      }

      if (cur_widget->widget_flags & widget_flag_draw_background)
      {
        Instance_Buffer_Element *draw_call = push_struct(&render->render_data, Instance_Buffer_Element);

        Persistent_Widget_Data *found_data = ui_search_persistent_data(cur_widget);
        f32 t = 1 - fast_powf(2.0f, -16.0f * ((f32) global_state->dt));

        // NOTE(antonio): I KNOW
       found_data->background_color[0] = lerp(found_data->background_color[0], t, cur_widget->end_background_color[0]);
       found_data->background_color[1] = lerp(found_data->background_color[1], t, cur_widget->end_background_color[1]);
       found_data->background_color[2] = lerp(found_data->background_color[2], t, cur_widget->end_background_color[2]);
       found_data->background_color[3] = lerp(found_data->background_color[3], t, cur_widget->end_background_color[3]);

        copy_memory_block(draw_call->color, found_data->background_color, sizeof(found_data->background_color));

        draw_call->size = 
        {
          0.0f, 0.0f,
          rect_get_width(&cur_widget->rectangle),
          rect_get_height(&cur_widget->rectangle),
        };

        draw_call->uv =
        {
          (f32) render->atlas->solid_color_rect.x0,
          (f32) render->atlas->solid_color_rect.y0,
          (f32) render->atlas->solid_color_rect.x1,
          (f32) render->atlas->solid_color_rect.y1,
        };

        draw_call->pos =
        {
          cur_widget->rectangle.x0,
          cur_widget->rectangle.y0,
          0.5f,
        };
      }

      if (cur_widget->widget_flags & widget_flag_draw_text)
      {
        f32 x        = cur_widget->rectangle.x0 + ((f32) ui->text_gutter_dim.x);
        f32 baseline = cur_widget->rectangle.y0 + ui->text_height - ((f32) ui->text_gutter_dim.y);

        set_temp_arena_wait(1);
        render_draw_text(&x, &baseline, cur_widget->text_color, cur_widget->string.str);
      }

      first_child = NULL;
      for (Widget *cur_child = cur_widget->first_child;
           cur_child != first_child;
           cur_child = cur_child->next_sibling)
      {
        ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
        first_child = cur_widget->first_child;
      }
    }

    if (!hot_key_should_be_kept)
    {
      ui->hot_key = nil_key;
    }
  }
}
#define TRADER_UI_IMPL_H
#endif
