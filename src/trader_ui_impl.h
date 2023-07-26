#if !defined(TRADER_UI_IMPL_H)
internal void ui_make_widget(Widget_Flag        widget_flags,
                             Widget_Size_Flag   size_flags,
                             String_Const_utf8  string,
                             V2_f32             sizing,
                             V2_f32             position,
                             f32                corner_radius,
                             f32                edge_softness,
                             f32                border_thickness,
                             void              *data,
                             u64                data_size,
                             String_Const_utf8 *alt_key_source)
{
  // TODO(antonio): sprint nation?
  UI_Context     *ui     = ui_get_context();
  Render_Context *render = render_get_context();

  unused(data);
  unused(data_size);

  if (ui->current_widget_count < ui->max_widget_count)
  {
    Widget *widget = ui->widget_free_list_head;

    ++ui->current_widget_count;
    ui->widget_free_list_head = ui->widget_free_list_head->next_sibling;

    zero_struct(widget);

    // TODO(antonio): append_widget?
    Widget *cur_par = ui->current_panel_parent->current_parent;

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

    if (widget_flags & widget_flag_arbitrary_draw)
    {
      render_get_common_context()->vertex_render_dimensions = sizing;
    }

    widget->widget_flags = widget_flags;
    widget->size_flags   = size_flags;
    widget->string       = string;
    widget->text_color   = ui->text_color;
    widget->key          = alt_key_source == NULL ? ui_make_key(string) : ui_make_key(*alt_key_source);

    widget->position_relative_to_parent = position;
    widget->extra_sizing                = sizing;

    widget->corner_radius    = corner_radius;
    widget->edge_softness    = edge_softness;
    widget->border_thickness = border_thickness;

    copy_memory_block(widget->end_background_color, ui->background_color, sizeof(ui->background_color));
  }
}

internal void ui_adjust_widget(Widget *widget, Widget_Parameters *params)
{
  copy_struct(&widget->params, params);
}

internal inline Widget *ui_get_last_placed_widget()
{
  UI_Context *ui            = ui_get_context();
  Panel      *current_panel = ui->current_panel_parent;
  Widget     *last_placed   = current_panel->current_parent;
  return(last_placed);
}

internal Widget *ui_make_sentinel_widget()
{
  Widget     *sentinel     = NULL;
  UI_Context *ui           = ui_get_context();

  if (ui->current_widget_count < ui->max_widget_count)
  {
    sentinel = ui->widget_free_list_head;

    ++ui->current_widget_count;
    ui->widget_free_list_head = ui->widget_free_list_head->next_sibling;

    zero_struct(sentinel);
    sentinel->string = string_literal_init_type("sentinel widget", utf8);
  }

  return(sentinel);
}

// TODO(antonio): cannot have two directions in a children list
internal Panel *ui_make_panel(Axis_Split split, f32 *size_relative_to_parent, String_Const_utf8 string, Panel *from)
{
  Panel      *panel   = NULL;
  UI_Context *ui      = ui_get_context();
  u32 max_panel_count = ui->panel_memory_size / sizeof(Panel); 

  expect(ui    != NULL);
  expect(split != axis_split_none);
  expect(size_relative_to_parent != NULL);
  expect(is_between_inclusive(0.0f, *size_relative_to_parent, 1.0f));

  if (ui->panel_count < max_panel_count)
  {
    panel = ui->panel_free_list_head;

    ++ui->panel_count;
    ui->panel_free_list_head = ui->panel_free_list_head->next_sibling;

    Panel *cur_par = from == NULL ? ui->current_panel_parent : from;
    expect(cur_par != NULL);

    if (cur_par->first_child == NULL)
    {
      panel->next_sibling     = panel;
      panel->previous_sibling = panel;

      cur_par->first_child = panel;
      cur_par->last_child  = panel;
    }
    else
    {
      panel->previous_sibling = cur_par->last_child;
      panel->next_sibling     = cur_par->first_child;

      cur_par->last_child->next_sibling      = panel;
      cur_par->first_child->previous_sibling = panel;
      cur_par->last_child                    = panel;
    }

    String_Const_utf8 panel_prefix         = string_literal_init_type("Panel::", utf8);
    String_Const_utf8 panel_to_hash_string = concat_string_to_c_string(ui->string_pool, panel_prefix, string);

    panel->parent                  = cur_par;
    panel->split                   = split;
    panel->size_relative_to_parent = size_relative_to_parent;
    panel->string                  = panel_to_hash_string;
    panel->sizing_left             = 1.0f;

    panel->sentinel = panel->current_parent = ui_make_sentinel_widget();
    expect(panel->current_parent != NULL);

    panel->parent->sizing_left -= *size_relative_to_parent;
    // expect(0 <= panel->parent->sizing_left);

    ui->current_panel_parent = panel;

    Widget_Parameters new_params_just_in_case =
    {
      V2(0.0f, 0.0f),
      V2(1.0f, 1.0f),
      global_slider_float * 1000.0f,
      *panel->size_relative_to_parent,
      3.0f,
    };

    ui_make_widget(widget_flag_border_draggable | widget_flag_draw_background | widget_flag_top_level,
                   size_flag_copy_parent_size_x | size_flag_copy_parent_size_y, 
                   string,
                   new_params_just_in_case.sizing,
                   new_params_just_in_case.pos,
                   new_params_just_in_case.corner_radius,
                   *panel->size_relative_to_parent,
                   new_params_just_in_case.border_thickness);

    ui_push_parent(panel->current_parent->first_child);
  }

  return(panel);
}

internal Panel *ui_make_panels(Axis_Split split, f32 *sizes, String_Const_utf8 *strings, u32 count, Panel *to_split)
{
  UI_Context *ui = ui_get_context();
  if (to_split == NULL)
  {
    expect(ui->current_panel_parent != NULL);
    to_split = ui->current_panel_parent;
  }

  if (to_split->first_child != NULL)
  {
    expect_message(to_split->first_child->split == split, "Panels only accept one split direction");
  }

  for (u32 panel_index = 0;
       panel_index < count;
       ++panel_index)
  {
    ui_make_panel(split, &sizes[panel_index], strings[panel_index], to_split);
  }

  return(to_split->first_child);
}

internal inline UI_Context *ui_get_context()
{
  return(&platform_get_global_state()->ui_context);
}

internal inline void ui_add_key_event(Key_Event event, b32 is_down)
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

  if (is_down)
  {
    ring_buffer_append(&ui->event_queue, &event, sizeof(u32));
  }
}

internal inline i64 ui_key_event_to_utf8(Key_Event event, utf8 *put, u64 put_length)
{
  i64 result = -1;
  u32 to_encode = max_u32;

  UI_Context *ui    = ui_get_context();
  b8          shift = ui->mod_keys.shift;

  switch (event)
  {
    case key_event_tab:
    {
      to_encode = '\t';
    } break;
    case key_event_space:
    {
      to_encode = ' ';
    } break;
    case key_event_enter:
    {
      to_encode = '\n';
    } break;
    case key_event_apostrophe:
    {
      to_encode = shift ? '"' : '\'';
    } break;
    case key_event_comma:
    {
      to_encode = shift ? '<' : ',';
    } break;
    case key_event_minus:
    {
      to_encode = shift ? '_' : '-';
    } break;
    case key_event_period:
    {
      to_encode = shift ? '>' : '.';
    } break;
    case key_event_slash:
    {
      to_encode = shift ? '?' : '/';
    } break;
    case key_event_semicolon:
    {
      to_encode = shift ? ':' : ';';
    } break;
    case key_event_equal:
    {
      to_encode = shift ? '+' : '=';
    } break;
    case key_event_left_bracket:
    {
      to_encode = shift ? '{' : '[';
    } break;
    case key_event_backslash:
    {
      to_encode = shift ? '|' : '\\';
    } break;
    case key_event_right_bracket:
    {
      to_encode = shift ? '}' : ']';
    } break;
    case key_event_grave_accent:
    {
      to_encode = shift ? '~' : '`';
    } break;
    case key_event_keypad_0:
    case key_event_keypad_1:
    case key_event_keypad_2:
    case key_event_keypad_3:
    case key_event_keypad_4:
    case key_event_keypad_5:
    case key_event_keypad_6:
    case key_event_keypad_7:
    case key_event_keypad_8:
    case key_event_keypad_9:
    {
      to_encode = (event - key_event_keypad_0) + '0';
    } break;
    case key_event_keypad_enter:
    {
      to_encode = '\n';
    } break;
    case key_event_0:
    {
      to_encode = shift ? ')' : '0';
    } break;
    case key_event_1:
    {
      to_encode = shift ? '!' : '1';
    } break;
    case key_event_2:
    {
      to_encode = shift ? '@' : '2';
    } break;
    case key_event_3:
    {
      to_encode = shift ? '#' : '3';
    } break;
    case key_event_4:
    {
      to_encode = shift ? '$' : '4';
    } break;
    case key_event_5:
    {
      to_encode = shift ? '%' : '5';
    } break;
    case key_event_6:
    {
      to_encode = shift ? '^' : '6';
    } break;
    case key_event_7:
    {
      to_encode = shift ? '&' : '7';
    } break;
    case key_event_8:
    {
      to_encode = shift ? '*' : '8';
    } break;
    case key_event_9:
    {
      to_encode = shift ? '(' : '9';
    } break;
    case key_event_a:
    case key_event_b:
    case key_event_c:
    case key_event_d:
    case key_event_e:
    case key_event_f:
    case key_event_g:
    case key_event_h:
    case key_event_i:
    case key_event_j:
    case key_event_k:
    case key_event_l:
    case key_event_m:
    case key_event_n:
    case key_event_o:
    case key_event_p:
    case key_event_q:
    case key_event_r:
    case key_event_s:
    case key_event_t:
    case key_event_u:
    case key_event_v:
    case key_event_w:
    case key_event_x:
    case key_event_y:
    case key_event_z:
    {
      to_encode = (event - key_event_a) + (shift ? 'A' : 'a');
      break;
    }
  }

  if (to_encode != max_u32)
  {
    result = unicode_utf8_encode(&to_encode, 1, put, 0, put_length);
  }

  return(result);
}

internal void ui_initialize(UI_Context *ui)
{
  Arena *global_arena = platform_get_global_arena();

  // NOTE(antonio): ui init
  ui->widget_memory      = push_array_zero(global_arena, Widget, default_widget_count);
  ui->widget_memory_size = sizeof(Widget) * default_widget_count;

  ui->string_pool  = push_struct(global_arena, Arena);
  *ui->string_pool =arena_alloc(default_string_pool_size, 1, NULL);

  // TODO(antonio): do I need to do back links?
  Widget *free_widget_list = ui->widget_memory;
  for (u64 widget_index = 0;
       widget_index < default_widget_count - 1;
       ++widget_index)
  {
    free_widget_list[widget_index].next_sibling = &free_widget_list[widget_index + 1];
    free_widget_list[widget_index].string       = string_literal_init_type("no widgets here, buddy :)", utf8);
  }

  Panel *panel_memory = push_array_zero(global_arena, Panel, default_panel_count);
  for (u32 panel_index = 0;
       panel_index < default_panel_count - 1;
       ++panel_index)
  {
    panel_memory[panel_index].next_sibling = &panel_memory[panel_index + 1];
  }

  ui->panels_start      = panel_memory;
  ui->panel_memory_size = default_panel_count * sizeof(*ui->panels_start);
  ui->panel_count       = 0;

  ui->event_queue.start = ui->event_queue.read = ui->event_queue.write = (u8 *) __event_queue_buffer;
  ui->event_queue.size  = array_count(__event_queue_buffer);

  for (u32 layer_index = 0;
       layer_index < array_count(ui->render_layers);
       ++layer_index)
  {
    ui->render_layers[layer_index] = arena_alloc(render_data_size / 4, 1, NULL);
  }
};

internal void ui_initialize_frame(void)
{
  UI_Context *ui = ui_get_context();

  arena_reset_zero(ui->string_pool);

  {
    zero_memory_block(ui->widget_memory, sizeof(Widget) * ui->current_widget_count);

    for (u32 widget_index = 0;
         widget_index < ui->current_widget_count;
         ++widget_index)
    {
      Widget *current_free       = &ui->widget_memory[widget_index];
      current_free->next_sibling = &ui->widget_memory[widget_index + 1];
      current_free->string       = string_literal_init_type("you reset this one", utf8);
    }

    ui->max_widget_count       = (u32) (ui->widget_memory_size / sizeof(Widget));
    ui->current_widget_count   = 1;

    ui->widget_free_list_head  = ui->widget_memory + 1;
    ui->allocated_widgets      = ui->widget_memory;
  }

  {
    zero_memory_block(ui->panels_start, sizeof(Panel) * ui->panel_count);
    for (u32 panel_index = 0;
         panel_index < ui->panel_count;
         ++panel_index)
    {
      Panel *current_free        = &ui->panels_start[panel_index];
      current_free->next_sibling = current_free + 1;
      current_free->split        = axis_split_none;
      current_free->sizing_left  = 1.0f;
    }

    Panel *sentinel_panel       = ui->panels_start;
    sentinel_panel->string      = string_literal_init_type("sentinel panel", utf8);
    sentinel_panel->sizing_left = 1.0f;
    ui->current_panel_parent    = sentinel_panel;

    ui->panel_free_list_head    = ui->panels_start + 1;
    ui->panel_count             = 1;
  }

  ui->text_height     = default_text_height;
  ui->text_gutter_dim = default_text_gutter_dim;
  ui->text_color      = default_text_color;

  copy_memory_block(ui->background_color, (void *) default_background_color, sizeof(default_background_color));

  ui->canvas_viewport = {};
  ui->keep_hot_key    = false;
  ui->keep_active_key = false;

  default_persistent_data    = {};

  for (u32 layer_index = 0; layer_index < array_count(ui->render_layers); ++layer_index)
  {
    arena_reset(&ui->render_layers[layer_index]);
  }
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

internal inline void ui_push_background_color(RGBA_f32 color)
{
  ui_push_background_color(color.r, color.g, color.b, color.a);
}

internal inline void ui_push_background_color(f32 r, f32 g, f32 b, f32 a)
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

internal void ui_pop_background_color(void)
{
  UI_Context *ui = ui_get_context();
  copy_memory_block(ui->background_color, (void *) default_background_color, sizeof(default_background_color));
}

internal void ui_push_parent(Widget *widget)
{
  UI_Context *ui = ui_get_context();
  ui->current_panel_parent->current_parent = widget;
}

internal void ui_pop_parent(void)
{
  Panel *panel_parent = ui_get_context()->current_panel_parent;
  if (panel_parent->current_parent->parent)
  {
    panel_parent->current_parent = panel_parent->current_parent->parent;
  }
}

internal inline void ui_push_panel_parent(Panel *new_parent)
{
  UI_Context *ui = ui_get_context();
  ui->current_panel_parent = new_parent;
}

internal inline void ui_pop_panel_parent(void)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  if (panel_parent->parent)
  {
    ui->current_panel_parent = panel_parent->parent;
  }
}

// TODO(antonio): consider what to do for format and string
internal void ui_do_string(String_Const_utf8 string)
{
  UI_Context *ui = ui_get_context();
  String_Const_utf8 copy_string = copy_str(ui->string_pool, string);

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content | size_flag_advancer_y,
                 copy_string);
}

internal void ui_do_formatted_string(char *format, ...)
{
  UI_Context *ui = ui_get_context();

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
                 size_flag_text_content | size_flag_advancer_y,
                 sprinted_text);
}

internal inline b32 ui__update_text_edit_bounds(Text_Range *range,
                                                UI_Event    event,
                                                i64         index,
                                                V2_f32      mouse,
                                                V2_f32      initial,
                                                f32        *bounds)
{
  f32 first_half   = lerpf(bounds[0], 0.6f, bounds[1]);
  f32 second_half  = lerpf(bounds[1], 0.6f, bounds[2]);

  if (is_between_inclusive(first_half, mouse.x, second_half))
  {
    range->start_index = index;

    if ((event & ui_event_drag) == 0)
    {
      range->inclusive_end_index = index;
    }
  }

  if ((range->start_index != -1) && (range->inclusive_end_index != -1))
  {
    return(true);
  }

  if (is_between_inclusive(first_half, initial.x, second_half))
  {
    range->inclusive_end_index = index;
  }

  if ((range->start_index != -1) && (range->inclusive_end_index != -1))
  {
    return(true);
  }
  else
  {
    return(false);
  }
}

internal void ui_do_text_edit(Text_Edit_Buffer *teb, char *format, ...)
{
  expect(teb != NULL);
  UI_Context *ui = ui_get_context();

  String_Const_utf8 copy_string = copy_str(ui->string_pool, teb->buf);

  va_list args;
  va_start(args, format);

  String_Const_utf8 sprinted_text;

  char *string_start = (char *) (ui->string_pool->start + ui->string_pool->used);
  sprinted_text.size = stbsp_vsnprintf(string_start, 512, format, args);
  sprinted_text.str  = (utf8 *) string_start;

  arena_push(ui->string_pool, sprinted_text.size + 1);

  va_end(args);

  ui_make_widget(widget_flag_draw_text  | widget_flag_get_user_input | widget_flag_clickable | widget_flag_draggable,
                 size_flag_text_content | size_flag_advancer_y,
                 copy_string,
                 V2(1.0f, 1.0f),
                 V2(0.0f, 0.0f),
                 0.0f,
                 0.0f,
                 0.0f,
                 NULL,
                 0,
                 &sprinted_text);

  UI_Key widget_key = ui_make_key(sprinted_text);
  for (u32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index) 
  {
    UI_Interaction *cur_int = &ui->interactions[interaction_index];
    if (cur_int->key == widget_key)
    {
      UI_Event_Value *value = &cur_int->value;
      if ((cur_int->event & ui_event_mouse) || (cur_int->event & ui_event_drag))
      {
        Common_Render_Context *render            = render_get_common_context();
        stbtt_packedchar      *packed_char_start = render->atlas->char_data + render_get_packed_char_start(ui->text_height);

        Widget *text_edit_widget = ui->current_panel_parent->current_parent->last_child;

        V2_f32 scaled_mouse    = hadamard_product(text_edit_widget->computed_size_in_pixels, value->mouse);
        V2_f32 scaled_initial  = hadamard_product(text_edit_widget->computed_size_in_pixels, value->mouse_initial_pos);

        Text_Range text_range = {-1, -1};
        f32        bounds[3] = {};

        i64 string_index      = 0;

        /*
        stbtt_packedchar *cur_packed_char = packed_char_start + (teb->buf.data[string_index] - starting_code_point);
        bounds[2] = cur_packed_char->xadvance;
        */
        stbtt_packedchar *cur_packed_char = NULL;

        i64 one_char_before_end = unicode_utf8_advance_char_pos(teb->buf.data, teb->buf.used, teb->buf.used, -1);
        while ((teb->buf.data[string_index] != '\0') && (string_index <= one_char_before_end))
        {
          // TODO(antonio): deal with new lines more gracefully
          if (is_newline(teb->buf.data[string_index])) 
          {
            continue;
          }
          else
          {
            cur_packed_char = packed_char_start + (teb->buf.data[string_index] - starting_code_point);

            bounds[0]  = bounds[1];
            bounds[1]  = bounds[2];
            bounds[2] += cur_packed_char->xadvance;

            if (ui__update_text_edit_bounds(&text_range, cur_int->event, string_index, scaled_mouse, scaled_initial, bounds))
            {
              break;
            }

            string_index += unicode_utf8_encoding_length(teb->buf.data + string_index);
          }
        }

        if (text_range.inclusive_end_index == -1)
        {
          text_range.inclusive_end_index = teb->buf.used;
        }

        if (text_range.start_index == -1)
        {
          text_range.start_index = teb->buf.used;
        }

        if (text_range.start_index > text_range.inclusive_end_index)
        {
          swap(i64, text_range.start_index, text_range.inclusive_end_index);
        }

        teb->range = text_range;
      }
      else if (cur_int->event & ui_event_keyboard)
      {
        utf8 *char_data   = value->utf8_data;
        u32   utf8_length = value->utf8_length;
        b32   control     = value->mod_keys.control;

        if ((utf8_length > 0) && !control)
        {
          if (range_get_length(&teb->range) > 0)
          {
            text_edit_delete(teb, -1);
          }

          String_utf8 to_insert = {char_data, utf8_length, utf8_length};
          text_edit_insert_string_and_advance(teb, to_insert);
        }
        else
        {
          b32 keep_selection = value->mod_keys.shift;

          switch (value->key_event)
          {
          case key_event_delete:
          case key_event_backspace:
            {
              i32 dir = (value->key_event == key_event_delete) ? 1 : -1;

              if (control)
              {
                text_edit_move_selection(teb, dir, true, text_edit_movement_word);
              }

              text_edit_delete(teb, dir);
            } break;
          case key_event_left_arrow:
          case key_event_right_arrow:
            {
              i32 dir = (value->key_event == key_event_left_arrow) ? -1 : 1;

              Text_Edit_Movement movement = control ? text_edit_movement_word : text_edit_movement_single;
              text_edit_move_selection(teb, (i64) dir, keep_selection, movement);
            } break;

          case key_event_home:
          case key_event_end:
            {
              i32 dir = (value->key_event == key_event_home) ? -1 : 1;
              text_edit_move_selection(teb, dir, keep_selection, text_edit_movement_end);
            } break;

          case key_event_v:
            {
              if (range_get_length(&teb->range) > 0)
              {
                text_edit_delete(teb, -1);
              }

              String_utf8 clipboard_data = su8(platform_read_clipboard_contents(ui->string_pool));
              text_edit_insert_string_and_advance(teb, clipboard_data);
            } break;
          case key_event_c:
            {
              String_utf8 data_to_set;
              u64 range_length = range_get_length(&teb->range);

              if (range_length > 0)
              {
                data_to_set = {teb->buf.data + teb->range.start_index, range_length, range_length};
                platform_write_clipboard_contents(data_to_set);
              }
            } break;
          }
        }
      }
    }
  }
}

internal b32 ui_do_button(String_Const_utf8 string)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;
  b32         result       = false;

  String_Const_utf8 button_parent_to_hash_prefix = string_literal_init_type("Button parent::", utf8);
  String_Const_utf8 button_parent_to_hash = concat_string_to_c_string(ui->string_pool, button_parent_to_hash_prefix, string);

  ui_make_widget(widget_flag_draw_background | widget_flag_clickable |
                 widget_flag_hot_animation   | widget_flag_active_animation,
                 size_flag_text_content | size_flag_advancer_y,
                 button_parent_to_hash,
                 V2(1.0f, 1.0f),
                 V2(0.0f, 0.0f),
                 4.0f, 0.6f, 1.0f);

  Widget *button_text_parent = panel_parent->current_parent->last_child;
  ui_push_parent(button_text_parent);

  String_Const_utf8 copy_string = copy_str(ui->string_pool, string);

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content | size_flag_relative_to_parent_pos_y,
                 copy_string);

  ui_pop_parent();
  ui_push_parent(last_parent);

  for (u32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index) 
  {
    if (ui->interactions[interaction_index].key == button_text_parent->key)
    {
      result = true;
      break;
    }
  }

  /*
  button_text_parent->end_background_color[0] = rgba(0.0f, 0.0f, 0.0f, 1.0f);
  button_text_parent->end_background_color[1] = rgba(1.0f, 1.0f, 1.0f, 1.0f);
  button_text_parent->end_background_color[2] = rgba(0.0f, 0.0f, 0.0f, 1.0f);
  button_text_parent->end_background_color[3] = rgba(1.0f, 1.0f, 1.0f, 1.0f);
  */

  return(result);
}

internal void ui_do_slider_f32(String_Const_utf8 string, f32 *in_out_value, f32 minimum, f32 maximum)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;

  expect(in_out_value != NULL);
  expect((minimum <= *in_out_value) && (*in_out_value <= maximum));
  // expect(compare_string_utf8(last_parent->string, panel_parent->sentinel->string));

  String_Const_utf8 slider_parent_to_hash_prefix = string_literal_init_type("Slider parent::", utf8);
  String_Const_utf8 slider_parent_to_hash = concat_string_to_c_string(ui->string_pool, slider_parent_to_hash_prefix, string);

  ui_make_widget(widget_flag_none,
                 size_flag_copy_parent_size_x | size_flag_given_size_y | size_flag_advancer_y,
                 slider_parent_to_hash,
                 V2(0.5f, ui->text_height));

  Widget *slider_parent = panel_parent->current_parent->last_child;
  ui_push_parent(slider_parent);

  ui_push_background_color(*in_out_value, *in_out_value, 0.0f, 1.0f);

  slider_parent_to_hash.size--;
  String_Const_utf8 slider_to_hash = concat_string_to_c_string(ui->string_pool,
                                                               slider_parent_to_hash,
                                                               string_literal_init_type("::slider", utf8));

  f32 slider_width_scale = 0.05f;
  f32 norm = 1.0f / (maximum - minimum);
  f32 slider_x_scale = lerpf(0.0f, clamp(minimum, *in_out_value, maximum) * norm, 1.0f);

  ui_make_widget(widget_flag_draw_background  | widget_flag_draggable,
                 size_flag_copy_parent_size_x | size_flag_copy_parent_size_y |
                 size_flag_relative_to_parent_pos_x | size_flag_relative_to_parent_pos_y,
                 slider_to_hash,
                 V2(slider_width_scale, 1.0f), 
                 V2(slider_x_scale, 0.0f));

  Widget *slider = panel_parent->current_parent->last_child;
  ui_pop_background_color();

  ui_pop_parent();
  ui_push_parent(last_parent);

  for (u32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index)
  {
    UI_Interaction *cur_int = ui->interactions + interaction_index;
    if (cur_int->key == slider->key)
    {
      f32 delta_x   = lerpf(minimum, cur_int->value.mouse.x, maximum);
      *in_out_value = clamp(minimum, delta_x, maximum);
      platform_set_cursor(cursor_kind_left_right_direction);
    }
  }
}

internal void ui_canvas(String_Const_utf8 string, V2_f32 size)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;

  // expect(compare_string_utf8(last_parent->string, panel_parent->sentinel->string));
  ui_push_background_color(1.0f, 0.0f, 0.0f, 1.0f);

  ui_make_widget(widget_flag_arbitrary_draw,
                 size_flag_given_size_x | size_flag_given_size_y | size_flag_advancer_y,
                 string, 
                 size,
                 V2(0.0f, 0.0f));

  ui_pop_background_color();

  ui_pop_parent();
  ui_push_parent(last_parent);
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

internal inline Panel *ui_get_sentinel_panel()
{
  UI_Context *ui       = ui_get_context();
  Panel      *sentinel = ui->panels_start;
  return(sentinel);
}
internal void ui_evaluate_child_sizes_panel(Panel *panel)
{
  if (panel->first_child == panel->last_child) return;

  UI_Context *ui = ui_get_context();

  // f32 prev_acc = 0.0f;
  f32 accumulated = 0.0f;
  // f32 next_acc = 0.0f;

  for (Panel *cur_child = panel->first_child;
       cur_child != panel->last_child;
       cur_child  = cur_child->next_sibling)
  {
    UI_Key panel_widget_key      = cur_child->sentinel->first_child->key;
    UI_Key next_panel_widget_key = cur_child->next_sibling->sentinel->first_child->key;

    accumulated += *cur_child->size_relative_to_parent;
    //next_acc     = accumulated + *cur_child->next_sibling->size_relative_to_parent;

    for (u32 interaction_index = 0;
         interaction_index < array_count(ui->interactions);
         ++interaction_index)
    {
      UI_Interaction *cur_int = ui->interactions + interaction_index;

      Rectangle_Side side = cur_int->value.extra_data;
      Rectangle_Side cur_side_needed  =
        (cur_child->split == axis_split_vertical) ? rectangle_side_right : rectangle_side_down;
      Rectangle_Side next_side_needed = 
        (cur_child->split == axis_split_vertical) ? rectangle_side_left : rectangle_side_up;

      b32 got_cur  = ((cur_int->key == panel_widget_key)      && (side == cur_side_needed));
      b32 got_next = ((cur_int->key == next_panel_widget_key) && (side == next_side_needed));

      if (got_cur || got_next)
      {
        f32 mouse_val = (cur_child->split == axis_split_vertical) ? cur_int->value.mouse.x : cur_int->value.mouse.y;
        f32 delta     = lerpf(0.0, mouse_val, 1.0f);

        f32 from_original = delta - accumulated;

        f32 cur_size  = *cur_child->size_relative_to_parent;
        f32 next_size = *cur_child->next_sibling->size_relative_to_parent;

        if (((cur_size  + from_original) >= smallest_panel_size) &&
            ((next_size - from_original) >= smallest_panel_size))
        {
          *cur_child->size_relative_to_parent               += from_original;
          *cur_child->next_sibling->size_relative_to_parent -= from_original;
        }

        if (cur_child->split == axis_split_vertical)
        {
          platform_set_cursor(cursor_kind_left_right_direction);
        }
        else
        {
          platform_set_cursor(cursor_kind_up_down_direction);
        }

        return;
      }

      //prev_acc = accumulated;
    }
  }
}

internal inline Arena *ui_get_render_layer(u32 layer)
{
  UI_Context *ui = ui_get_context();
  expect(is_between_inclusive(0, layer, array_count(ui->render_layers) - 1));

  Arena *render_layer = &ui->render_layers[layer];
  return(render_layer);
}

internal void ui_flatten_draw_layers(void)
{
  UI_Context            *ui     = ui_get_context();
  Common_Render_Context *render = render_get_common_context();

  u32 draw_call_count = 0;
  for (u32 layer_index = 0;
       layer_index < array_count(ui->render_layers);
       ++layer_index)
  {
    draw_call_count += (u32) ui->render_layers[layer_index].used / sizeof(Instance_Buffer_Element);
  }

  Instance_Buffer_Element *flattened_elements = push_array(&render->render_data, Instance_Buffer_Element, draw_call_count);

  expect(flattened_elements != NULL);
  expect(draw_call_count    <= (render->render_data.size - render->render_data.used));

  u32 layer_start_index = (u32) (((u8 *) flattened_elements - render->render_data.start) / sizeof(*flattened_elements));

  for (u32 layer_index = 0;
       layer_index < array_count(ui->render_layers);
       ++layer_index)
  {
    Instance_Buffer_Element *instances      = (Instance_Buffer_Element *) ui->render_layers[layer_index].start;
    u32                      instance_count = (u32) (ui->render_layers[layer_index].used / sizeof(*instances));

#define TRADER__FLATTEN_DEBUG 0
#if TRADER__FLATTEN_DEBUG
    for (u32 instance_index = 0; instance_index < instance_count; ++instance_index)
    {
      Instance_Buffer_Element *cur_inst; cur_inst = instances + instance_index;
      int i = 0; i++;
    }
#endif

    copy_memory_block(flattened_elements, instances, ui->render_layers[layer_index].used);

#if TRADER__FLATTEN_DEBUG
    for (u32 instance_index = 0; instance_index < instance_count; ++instance_index)
    {
      Instance_Buffer_Element *cur_inst; cur_inst = flattened_elements + instance_index;
      int i = 0; i++;
    }
#endif

    ui->flattened_draw_layer_indices[layer_index]  = layer_start_index;
    layer_start_index                             += instance_count;

    flattened_elements                            += instance_count;
  }
}

internal inline void ui_add_interaction(Widget *cur_widget, i32 frames_left, u32 event, UI_Event_Value *event_value)
{
  UI_Context *ui = ui_get_context();

  b32 was_nil = false;
  i32 interaction_update_index = -1;

  for (i32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index) 
  {
    UI_Interaction *cur_int = &ui->interactions[interaction_index];
    if ((cur_int->key == cur_widget->key) || (cur_int->key == nil_key))
    {
      interaction_update_index = interaction_index;
      was_nil = (cur_int->key == nil_key);

      break;
    }
  }

  if (interaction_update_index != -1)
  {
    UI_Interaction *cur_int = ui->interactions + interaction_update_index;
    cur_int->key            = cur_widget->key;

    cur_int->frames_left    = frames_left;
    cur_int->frames_active  = was_nil ? 0 : cur_int->frames_active + 1;

    cur_int->event          = cur_int->event ? cur_int->event : event;

    V2_f32 mouse_initial = cur_int->value.mouse_initial_pos;
    copy_struct(&cur_int->value, event_value);

    if ((event & ui_event_drag) || (event & ui_event_mouse))
    {
      cur_int->value.mouse_initial_pos = mouse_initial;
    }

    const u32 frames_until_drag = 10;
    if ((event & ui_event_drag) && (cur_widget->widget_flags & widget_flag_clickable))
    {
      if ((cur_int->frames_active < frames_until_drag) && ((event & ui_event_keyboard) == 0))
      {
        cur_int->event = ui_event_none;
      }

      if ((cur_int->value.mouse_initial_pos.x == 0.0f) && 
          (cur_int->value.mouse_initial_pos.y == 0.0f))
      {
        cur_int->value.mouse_initial_pos = event_value->mouse;
      }
    }

  }
}

internal void ui_prepare_render_from_panels(Panel *panel, Rect_f32 rect)
{
  if (panel == NULL) {
    return;
  }

  Common_Render_Context *render = render_get_common_context();
  V2_f32 rect_dimensions = rect_get_dimensions(&rect);

  Arena *temp_arena     = get_temp_arena();
  u64    remaining_size = arena_get_remaining_size(temp_arena);
  
  Ring_Buffer panel_queue = ring_buffer_make(temp_arena, structs_in_size(remaining_size / 2, Panel *));

  Panel *first_child = NULL;
  for (Panel *cur_child = panel->first_child;
       cur_child != first_child;
       cur_child  = cur_child->next_sibling)
  {
    ring_buffer_append(&panel_queue, &cur_child, sizeof(Widget *));
    first_child = panel->first_child;
  }
  
  ui_evaluate_child_sizes_panel(panel);

  RGBA_f32 start_color = rgba_from_u8(0, 0, 0, 255);
  RGBA_f32 end_color   = rgba_from_u8(255, 255, 255, 255);

  while (panel_queue.read != panel_queue.write)
  {
    Panel *cur_panel = NULL;
    ring_buffer_pop_and_put(&panel_queue, &cur_panel, sizeof(Panel **));

    Rect_f32 to_place = rect;
    f32      panel_sizing = *cur_panel->size_relative_to_parent;

    expect(cur_panel->split != axis_split_none);
    expect(is_between_inclusive(0.0f, panel_sizing, 1.0f));

    if (cur_panel->split == axis_split_horizontal)
    {
      to_place.y1 = rect.y0 + lerpf(0.0f, panel_sizing, rect_dimensions.y);
    }
    else
    {
      to_place.x1 = rect.x0 + lerpf(0.0f, panel_sizing, rect_dimensions.x);
    }

    cur_panel->sentinel->rectangle = to_place;
    cur_panel->sentinel->computed_size_in_pixels = rect_get_dimensions(&to_place);

    if (cur_panel->first_child == NULL)
    {
      // NOTE(antonio): need to remove draw call
      Arena *background_render_layer = ui_get_render_layer(0);
      Instance_Buffer_Element *draw_call = push_struct(background_render_layer, Instance_Buffer_Element);

      draw_call->size  = {0.0f, 0.0f, rect_get_width(&to_place), rect_get_height(&to_place)};
      draw_call->uv    =
      {
        (f32) render->atlas->solid_color_rect.x0,
        (f32) render->atlas->solid_color_rect.y0,
        (f32) render->atlas->solid_color_rect.x1,
        (f32) render->atlas->solid_color_rect.y1,
      };
      draw_call->pos      = V3(to_place.x0, to_place.y0, 0.4f);

      draw_call->color[0] = start_color;
      draw_call->color[1] = start_color;
      draw_call->color[2] = start_color;
      draw_call->color[3] = start_color;

      start_color = wide_lerp(start_color, 0.5f, end_color);

      draw_call->corner_radius    = global_slider_float * 1000.0f;
      draw_call->border_thickness = 3.0f;
      draw_call->edge_softness    = 0.5f;

      u64 initial_used = temp_arena->used;
      ui_prepare_render(cur_panel, cur_panel->sentinel, to_place);
      temp_arena->used = initial_used;
    }

    if (cur_panel->first_child != NULL)
    {
      first_child = NULL;
      for (Panel *cur_child = cur_panel->first_child;
           cur_child != first_child;
           cur_child  = cur_child->next_sibling)
      {
        ring_buffer_append(&panel_queue, &cur_child, sizeof(Widget *));
        first_child = cur_panel->first_child;
      }

      ui_evaluate_child_sizes_panel(cur_panel);
    }

    if (cur_panel->split == axis_split_horizontal)
    {
      rect.y0 = to_place.y1;
    }
    else
    {
      rect.x0 = to_place.x1;
    }
  }
}

internal void ui_prepare_render(Panel *panel, Widget *widgets, Rect_f32 rect)
{
  Global_Platform_State *global_state = platform_get_global_state();
  Arena                 _temp_arena   = get_rest_of_temp_arena(0.5f);
  Arena                 *temp_arena   = &_temp_arena;
  UI_Context            *ui           = ui_get_context();
  Render_Context        *render       = render_get_context();

  u64 ring_buffer_size = temp_arena->size;

  expect_message(compare_string_utf8(widgets->string, string_literal_init_type("sentinel widget", utf8)),
                 "expected first widget to be sentinel widget");
  {
    // NOTE(antonio): stack grows from high to low
    Arena *widget_stack = temp_arena;

    Widget **data = &widgets;
    arena_append(widget_stack, data, sizeof(Widget *));

    // NOTE(antonio): first-child biased loop
    Widget **stack_top  = NULL;
    Widget  *cur_widget = NULL;
    while ((stack_top = arena_get_top(widget_stack, Widget *)))
    {
      cur_widget = *stack_top;
      arena_pop(widget_stack, sizeof(Widget **));

      Widget *parent = cur_widget->parent;

      if (cur_widget->size_flags & size_flag_copy_parent_size_x)
      {
        expect(0.0f < cur_widget->extra_sizing.x);
        cur_widget->computed_size_in_pixels.x =
          parent->computed_size_in_pixels.x * cur_widget->extra_sizing.x;
      }

      if (cur_widget->size_flags & size_flag_copy_parent_size_y)
      {
        expect(0.0f < cur_widget->extra_sizing.y);
        cur_widget->computed_size_in_pixels.y =
          parent->computed_size_in_pixels.y * cur_widget->extra_sizing.y;
      }

      if (cur_widget->size_flags & size_flag_fill_rest_of_axis_x)
      {
        cur_widget->size_flags |= size_flag_to_be_sized_x;
      }

      if (cur_widget->size_flags & size_flag_fill_rest_of_axis_y)
      {
        cur_widget->size_flags |= size_flag_to_be_sized_y;
      }

      if (cur_widget->size_flags & size_flag_given_size_x)
      {
        cur_widget->computed_size_in_pixels.x = cur_widget->extra_sizing.x;
      }

      if (cur_widget->size_flags & size_flag_given_size_y)
      {
        cur_widget->computed_size_in_pixels.y = cur_widget->extra_sizing.y;
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

  arena_reset(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(ring_buffer_size, Widget **));

    // NOTE(antonio): don't care about the sentinel
    Widget *first_child = NULL;
    for (Widget *cur_child  = widgets->first_child;
         cur_child         != first_child;
         cur_child          = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = widgets->first_child;
    }

    V2_f32 cur_top_left = rect_get_top_left(&rect);
    // rect_get_top_left(&ui_get_sentinel()->rectangle);

    // NOTE(antonio): resolve "implicit" sizes with current information, level-order 
    while (widget_queue.write != widget_queue.read)
    {
      Widget *cur_widget = NULL;
      ring_buffer_pop_and_put_struct(&widget_queue, &cur_widget);

      V2_f32 pre_sizing_top_left = cur_top_left;

      f32 remaining_width = rect_get_width(&cur_widget->rectangle);
      f32 max_height = 0.0f;

      u32 to_be_sized_x = 0;

      {
        Widget *parent = cur_widget->parent;
        expect(parent != NULL);

        if (cur_widget->size_flags & size_flag_relative_to_parent_pos_y)
        {
          pre_sizing_top_left.y = parent->rectangle.y0 + cur_widget->position_relative_to_parent.y;
        }
      }

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

      // NOTE(antonio): place the children
      if (to_be_sized_x > 0)
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

          if (cur_child->size_flags & size_flag_to_be_sized_y)
          {
            cur_child->computed_size_in_pixels.y = max_height;
          }
        }
      }

      Widget *widget_parent = cur_widget->parent;

      if (cur_widget->size_flags & size_flag_relative_to_parent_pos_x)
      {
        cur_widget->rectangle.x0 =
          widget_parent->rectangle.x0 +
          (cur_widget->position_relative_to_parent.x * widget_parent->computed_size_in_pixels.x);
      }
      else
      {
        cur_widget->rectangle.x0 = pre_sizing_top_left.x + cur_widget->position_relative_to_parent.x;
      }

      if (cur_widget->size_flags & size_flag_relative_to_parent_pos_y)
      {
        cur_widget->rectangle.y0 =
          widget_parent->rectangle.y0 +
          (cur_widget->position_relative_to_parent.y * widget_parent->computed_size_in_pixels.y);
      }
      else
      {
        cur_widget->rectangle.y0 = pre_sizing_top_left.y + cur_widget->position_relative_to_parent.y;
      }

      cur_widget->rectangle.x1 = cur_widget->rectangle.x0 + cur_widget->computed_size_in_pixels.x;
      cur_widget->rectangle.y1 = cur_widget->rectangle.y0 + cur_widget->computed_size_in_pixels.y;

      cur_top_left.x = rect.x0;
      cur_top_left.y = pre_sizing_top_left.y;

      if (cur_widget->size_flags & size_flag_advancer_y)
      {
        cur_top_left.y += cur_widget->computed_size_in_pixels.y;
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
  }

  arena_reset(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(ring_buffer_size, Widget *));
    Widget *first_child = NULL;
    for (Widget *cur_child = widgets->first_child;
         cur_child != first_child;
         cur_child  = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = widgets->first_child;
    }

    b32 keep_hot_key    = false;
    UI_Event_Value event_value;

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
        b32 mouse_left_went_up = mouse_left_change && ((ui->cur_frame_mouse_event & mouse_event_lclick) == 0);

        if (ui_is_key_equal(ui->active_key, cur_widget->key))
        {
          if (mouse_left_went_up)
          {
            if (ui_is_key_equal(ui->hot_key, cur_widget->key))
            {
              Rect_f32 rect_to_use     = cur_widget->rectangle;
              V2_f32   rect_dimensions = rect_get_dimensions(&rect_to_use);

              event_value.mouse = V2((ui->mouse_pos.x - rect_to_use.x0) / rect_dimensions.x,
                                     (ui->mouse_pos.y - rect_to_use.y0) / rect_dimensions.y);
              ui_add_interaction(cur_widget, 1, ui_event_mouse, &event_value);
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

        if (rect_is_point_inside(ui->mouse_pos, cur_widget->rectangle))
        {
          if (ui->active_key == nil_key)
          {
            ui->hot_key = cur_widget->key;
          }
          keep_hot_key = true;
        }
      }

      if ((cur_widget->widget_flags & widget_flag_draggable) || 
          (cur_widget->widget_flags & widget_flag_border_draggable))
      {
        b32 border_draggable = (cur_widget->widget_flags & widget_flag_border_draggable) > 0;
        b32 draggable        = (cur_widget->widget_flags & widget_flag_draggable)        > 0;
        expect(draggable != border_draggable);

        b32 mouse_left_change = ((ui->prev_frame_mouse_event & mouse_event_lclick) !=
                                 (ui->cur_frame_mouse_event  & mouse_event_lclick));
        if (mouse_left_change)
        {
          b32 mouse_left_went_down = mouse_left_change && (ui->cur_frame_mouse_event & mouse_event_lclick);
          if (mouse_left_went_down)
          {
            if (ui->hot_key == cur_widget->key)
            {
              ui->active_key = cur_widget->key;
            }
          }
          else if (ui->active_key == cur_widget->key)
          {
            ui->active_key = nil_key;
          }
        }

        b32 make_hot       = rect_is_point_inside(ui->mouse_pos, cur_widget->rectangle);
        u32 drag_direction = 0;

        if (cur_widget->widget_flags & widget_flag_border_draggable)
        {
          Rect_f32 inner_rect              = cur_widget->rectangle;
          f32      scaled_border_thickness = 2.0f * cur_widget->border_thickness;

          inner_rect.x0 += scaled_border_thickness;
          inner_rect.y0 += scaled_border_thickness;

          inner_rect.x1 -= scaled_border_thickness;
          inner_rect.y1 -= scaled_border_thickness;

          make_hot = make_hot && !rect_is_point_inside(ui->mouse_pos, inner_rect);
          drag_direction = panel->split == axis_split_vertical ? 0 : 1;
        }

        if (make_hot)
        {
          if (ui->active_key == nil_key)
          {
            ui->hot_key = cur_widget->key;

            if ((cur_widget->widget_flags & widget_flag_clickable) == 0)
            {
              if (drag_direction == 0)
              {
                platform_set_cursor(cursor_kind_left_right_direction);
              }
              else
              {
                platform_set_cursor(cursor_kind_left_right_direction);
              }
            }
          }

          keep_hot_key = true;
        }

        if (ui->active_key == cur_widget->key)
        {
          Rect_f32 rect_to_use; 
          if (border_draggable)
          {
            rect_to_use = render_get_client_rect();
          }
          else if (cur_widget->widget_flags & widget_flag_draw_text)
          {
            rect_to_use = cur_widget->rectangle;
          } 
          else
          {
            rect_to_use = cur_widget->parent->rectangle;
          }

          V2_f32   rect_dimensions = rect_get_dimensions(&rect_to_use);

          event_value            = {};
          event_value.mouse      = V2((ui->mouse_pos.x - rect_to_use.x0) / rect_dimensions.x,
                                      (ui->mouse_pos.y - rect_to_use.y0) / rect_dimensions.y);
          event_value.extra_data = rect_get_closest_side_to_point(ui->mouse_pos, cur_widget->rectangle, rectangle_side_none);

          ui_add_interaction(cur_widget, 1, ui_event_drag, &event_value);
        }
      }

      if (cur_widget->widget_flags & widget_flag_get_user_input)
      {
        if (ui->event_queue.read != ui->event_queue.write)
        {
          Key_Event first_key_event;
          ring_buffer_pop_and_put(&ui->event_queue, &first_key_event, sizeof(first_key_event));

          zero_struct(&event_value);

          i64 encode_result =
            ui_key_event_to_utf8(first_key_event, (utf8 *) event_value.utf8_data, sizeof(event_value.utf8_data));

          if (encode_result > 0) 
          {
            event_value.utf8_length = (u32) encode_result;
          }
          else
          {
            event_value.utf8_length = 0;
          }

          event_value.mod_keys  = ui->mod_keys;
          event_value.key_event = first_key_event;
          ui_add_interaction(cur_widget, 1, ui_event_keyboard, &event_value);
        }
      }

      if (cur_widget->widget_flags & widget_flag_hot_animation)
      {
        if (ui->hot_key == cur_widget->key)
        {
          cur_widget->end_background_color[1] = rgba(1.0f, 1.0f, 1.0f, 1.0f);
          cur_widget->end_background_color[3] = rgba(1.0f, 1.0f, 1.0f, 1.0f);
        }
      }

      if (cur_widget->widget_flags & widget_flag_active_animation)
      {
        if (cur_widget->key == ui->active_key)
        {
          RGBA_f32 saved_background_color[4];
          copy_memory_block((void *) saved_background_color,
                            (void *) cur_widget->end_background_color,
                            sizeof(saved_background_color));

          cur_widget->end_background_color[0] = cur_widget->end_background_color[1];
          cur_widget->end_background_color[2] = cur_widget->end_background_color[3];

          cur_widget->end_background_color[1] = saved_background_color[0];
          cur_widget->end_background_color[3] = saved_background_color[2];
        }
      }

      if (cur_widget->widget_flags & widget_flag_draw_background)
      {
        Arena *background_render_layer = ui_get_render_layer(0);
        Instance_Buffer_Element *draw_call = push_struct(background_render_layer, Instance_Buffer_Element);

        Persistent_Widget_Data *found_data = ui_search_persistent_data(cur_widget);
        f32 t = 1 - fast_powf(2.0f, 16.0f * -((f32) global_state->dt));

        // NOTE(antonio): I KNOW
        found_data->background_color[0] = wide_lerp(found_data->background_color[0], t, cur_widget->end_background_color[0]);
        found_data->background_color[1] = wide_lerp(found_data->background_color[1], t, cur_widget->end_background_color[1]);
        found_data->background_color[2] = wide_lerp(found_data->background_color[2], t, cur_widget->end_background_color[2]);
        found_data->background_color[3] = wide_lerp(found_data->background_color[3], t, cur_widget->end_background_color[3]);

        RGBA_f32 color_bottom = rgba(0.0f, 0.0f, 0.0f, 0.0f);
        RGBA_f32 color_top    = rgba(1.0f, 1.0f, 1.0f, 1.0f);

        found_data->background_color[0] = wide_clamp(color_bottom, found_data->background_color[0], color_top);
        found_data->background_color[1] = wide_clamp(color_bottom, found_data->background_color[1], color_top);
        found_data->background_color[2] = wide_clamp(color_bottom, found_data->background_color[2], color_top);
        found_data->background_color[3] = wide_clamp(color_bottom, found_data->background_color[3], color_top);

        copy_memory_block(draw_call->color, found_data->background_color, sizeof(found_data->background_color));

        expect(rect.x0 <= rect.x1);
        expect(rect.y0 <= rect.y1);

        cur_widget->rectangle.p0 = wide_clamp(rect.p0, cur_widget->rectangle.p0, rect.p1);
        cur_widget->rectangle.p1 = wide_clamp(rect.p0, cur_widget->rectangle.p1, rect.p1);
        cur_widget->computed_size_in_pixels = rect_get_dimensions(&cur_widget->rectangle);

        // this is weird...
        draw_call->size = 
        {
          0.0f, 0.0f,
          cur_widget->computed_size_in_pixels.x - 0.5f,
          cur_widget->computed_size_in_pixels.y,
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
          cur_widget->widget_flags & widget_flag_top_level ? 0.25f : 0.5f,
        };

        draw_call->corner_radius    = cur_widget->corner_radius;
        draw_call->border_thickness = cur_widget->border_thickness;
        draw_call->edge_softness    = cur_widget->edge_softness;
      }

      if (cur_widget->widget_flags & widget_flag_draw_text)
      {
        f32 x        = cur_widget->rectangle.x0 + ((f32) ui->text_gutter_dim.x);
        f32 baseline = cur_widget->rectangle.y0 + ui->text_height - ((f32) ui->text_gutter_dim.y);

        Arena *text_render_layer = ui_get_render_layer(1);
        set_temp_arena_wait(1);
        render_draw_text(text_render_layer, &x, &baseline, cur_widget->text_color, rect, cur_widget->string.str);

        if (cur_widget->widget_flags & widget_flag_get_user_input)
        {
          Rect_f32 cursor = {};

          cursor.x0 = cur_widget->rectangle.x0 + ((f32) ui->text_gutter_dim.x);
          cursor.x1 = cursor.x0;

          cursor.y0 = cur_widget->rectangle.y0;
          cursor.y1 = cursor.y0 + ui->text_height;

          String_Const_utf8 teb_string = {debug_teb.buf.data, debug_teb.buf.used};
          render_get_text_dimensions(&cursor.x0, &cursor.y0, rect, teb_string, debug_teb.range.start_index - 1);

          i64 range_length = range_get_length(&debug_teb.range);
          if (range_length > 0)
          {
            render_get_text_dimensions(&cursor.x1,
                                       &cursor.y0,
                                       rect,
                                       teb_string,
                                       debug_teb.range.inclusive_end_index - 1);
          }
          else
          {
            cursor.x1 = cursor.x0 + 1.0f;
          }

          {
            Arena *over_text_layer = ui_get_render_layer(2);
            Instance_Buffer_Element *draw_call = push_struct(over_text_layer, Instance_Buffer_Element);
            zero_struct(draw_call);

            Rect_f32 solid_color_rect = {
              (f32) render->atlas->solid_color_rect.x0,
              (f32) render->atlas->solid_color_rect.y0,
              (f32) render->atlas->solid_color_rect.x1,
              (f32) render->atlas->solid_color_rect.y1,
            };

            draw_call->size = 
            {
              0.0f, 0.0f,
              rect_get_width(&cursor), rect_get_height(&cursor),
            };

            draw_call->pos = V3(cursor.x0, cursor.y0, 1.0f);

            RGBA_f32 cursor_color = (range_get_length(&debug_teb.range) == 0) ?
              rgba(1.0f, 1.0f, 1.0f, 1.0f) :
              rgba_from_u8(0, 204, 255, 180);

            draw_call->color[0] = cursor_color;
            draw_call->color[1] = cursor_color;
            draw_call->color[2] = cursor_color;
            draw_call->color[3] = cursor_color;

            draw_call->uv            = solid_color_rect;
            draw_call->edge_softness = 0.0f;
          }
        }
      }

      // TODO(antonio): is this right
      if (cur_widget->widget_flags & widget_flag_arbitrary_draw)
      {
        expect((ui->canvas_viewport.x0 == 0.0f) && (ui->canvas_viewport.y0 == 0.0f));
        expect((ui->canvas_viewport.x1 == 0.0f) && (ui->canvas_viewport.y1 == 0.0f));

        ui->canvas_viewport = cur_widget->rectangle;
      }

      first_child = NULL;
      for (Widget *cur_child = cur_widget->first_child;
           cur_child != first_child;
           cur_child = cur_child->next_sibling)
      {
        ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
        first_child = cur_widget->first_child;
      }

      ui->keep_hot_key = ui->keep_hot_key || keep_hot_key;
    }
  }

  arena_reset(temp_arena);
}
#define TRADER_UI_IMPL_H
#endif
