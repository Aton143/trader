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
                             u64                data_size)
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
    widget->key          = ui_make_key(string);

    widget->position_relative_to_parent = position;
    widget->extra_sizing                = sizing;

    widget->corner_radius    = corner_radius;
    widget->edge_softness    = edge_softness;
    widget->border_thickness = border_thickness;

    copy_memory_block(widget->end_background_color, ui->background_color, sizeof(ui->background_color));
  }
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
internal Panel *ui_make_panel(Axis_Split split, f32 size_relative_to_parent, String_Const_utf8 string, Panel *from)
{
  Panel      *panel   = NULL;
  UI_Context *ui      = ui_get_context();
  u32 max_panel_count = ui->panel_memory_size / sizeof(Panel); 

  expect(ui    != NULL);
  expect(split != axis_split_none);
  expect(is_between_inclusive(0.0f, size_relative_to_parent, 1.0f));

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

    panel->parent                  = cur_par;
    panel->split                   = split;
    panel->size_relative_to_parent = size_relative_to_parent;
    panel->string                  = string;
    panel->sizing_left             = 1.0f;

    panel->sentinel = panel->current_parent = ui_make_sentinel_widget();
    expect(panel->current_parent != NULL);

    panel->parent->sizing_left -= size_relative_to_parent;
    expect(0 <= panel->parent->sizing_left);

    ui->current_panel_parent = panel;
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
    ui_make_panel(split, sizes[panel_index], strings[panel_index], to_split);
  }

  return(to_split->first_child);
}

internal inline UI_Context *ui_get_context()
{
  return(&platform_get_global_state()->ui_context);
}

internal inline void ui_add_interaction(Widget *cur_widget, i32 frames_left, u32 event, UI_Event_Value *event_value)
{
  UI_Context *ui = ui_get_context();
  i32 interaction_update_index = -1;

  for (i32 interaction_index = 0;
       interaction_index < array_count(ui->interactions);
       ++interaction_index) 
  {
    UI_Interaction *cur_interaction = &ui->interactions[interaction_index];
    if ((cur_interaction->key == cur_widget->key) || (cur_interaction->key == nil_key))
    {
      interaction_update_index = interaction_index;
      break;
    }
  }

  if (interaction_update_index != -1)
  {
    UI_Interaction *cur_interaction = ui->interactions + interaction_update_index;

    cur_interaction->key         = cur_widget->key;
    cur_interaction->frames_left = frames_left;
    cur_interaction->event       = event;
    copy_struct(&cur_interaction->value, event_value);

#if !SHIP_MODE
    copy_struct(&cur_interaction->value2, &ui->mouse_pos);
    cur_interaction->start_frame = platform_get_global_state()->frame_count;
#endif
  }
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

  ui->key_events[event] = (b8) is_down;
}

internal void ui_initialize(UI_Context *ui)
{
  Arena *global_arena = platform_get_global_arena();

  // NOTE(antonio): ui init
  ui->widget_memory      = push_array_zero(global_arena, Widget, default_widget_count);
  ui->widget_memory_size = sizeof(Widget) * default_widget_count;

  ui->string_pool  = push_struct(global_arena, Arena);
  *ui->string_pool = arena_alloc(default_string_pool_size, 1, NULL);

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
  ui->keep_hot_key = false;

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
  UI_Context *ui           = ui_get_context();

  String_Const_utf8 copy_string;

  // NOTE(antonio): string pool gets cleared out every frame
  copy_string.str = (utf8 *) arena_push(ui->string_pool, string.size + 1);
  copy_memory_block(copy_string.str, string.str, string.size);
  copy_string.size = string.size + 1;

  ui_make_widget(widget_flag_draw_text,
                 size_flag_text_content,
                 copy_string);
}

internal void ui_do_formatted_string(char *format, ...)
{
  UI_Context *ui           = ui_get_context();

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
}

internal b32 ui_do_button(String_Const_utf8 string)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;
  b32         result       = false;

  String_Const_utf8 button_parent_to_hash_prefix = string_literal_init_type("Button parent::", utf8);
  String_Const_utf8 button_parent_to_hash = concat_string_to_c_string(ui->string_pool, button_parent_to_hash_prefix, string);

  ui_make_widget(widget_flag_draw_background | widget_flag_clickable,
                 size_flag_text_content,
                 button_parent_to_hash,
                 V2(1.0f, 1.0f),
                 V2(0.0f, 0.0f),
                 4.0f, 0.6f, 1.0f);

  Widget *button_text_parent = panel_parent->current_parent->last_child;
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
       interaction_index < array_count(ui->interactions);
       ++interaction_index) 
  {
    if (ui->interactions[interaction_index].key == button_text_parent->key)
    {
      result = true;
      break;
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
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;

  expect(in_out_value != NULL);
  expect((minimum <= *in_out_value) && (*in_out_value <= maximum));
  expect(compare_string_utf8(last_parent->string, panel_parent->sentinel->string));

  String_Const_utf8 slider_parent_to_hash_prefix = string_literal_init_type("Slider parent::", utf8);
  String_Const_utf8 slider_parent_to_hash = concat_string_to_c_string(ui->string_pool, slider_parent_to_hash_prefix, string);

  ui_make_widget(widget_flag_none,
                 size_flag_copy_parent_size_x | size_flag_given_size_y,
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

  ui_make_widget(widget_flag_draw_background  | widget_flag_dragable,
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
      f32 delta_x = lerpf(minimum, cur_int->value.mouse.x, maximum);
      *in_out_value = clamp(minimum, delta_x, maximum);
    }
  }
}

internal void ui_canvas(String_Const_utf8 string, V2_f32 size)
{
  UI_Context *ui           = ui_get_context();
  Panel      *panel_parent = ui->current_panel_parent;
  Widget     *last_parent  = panel_parent->current_parent;

  expect(compare_string_utf8(last_parent->string, panel_parent->sentinel->string));
  ui_push_background_color(1.0f, 0.0f, 0.0f, 1.0f);

  ui_make_widget(widget_flag_arbitrary_draw,
                 size_flag_given_size_x | size_flag_given_size_y,  
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

internal void ui_prepare_render_from_panels(Panel *panel, Rect_f32 rect)
{
  if (panel == NULL) {
    return;
  }

  Common_Render_Context *render = render_get_common_context();
  V2_f32  rect_dimensions = rect_get_dimensions(&rect);

  Arena *temp_arena  = get_temp_arena();
  Arena *panel_stack = temp_arena;

  Panel *first_child = NULL;
  for (Panel *cur_child  = panel->first_child;
       cur_child        != first_child;
       cur_child         = cur_child->next_sibling)
  {
    arena_append(panel_stack, &cur_child, sizeof(Panel *));
    first_child = panel->first_child;
  }

  Panel **stack_top = NULL;
  Panel  *cur_panel = NULL;
  while ((stack_top = arena_get_top(panel_stack, Panel *)))
  {
    cur_panel = *stack_top;

    Rect_f32 to_place = rect;

    expect(cur_panel->split != axis_split_none);
    expect(is_between_inclusive(0.0f, cur_panel->size_relative_to_parent, 1.0f));

    if (cur_panel->split == axis_split_horizontal)
    {
      to_place.y1 = rect.y0 + lerpf(0.0f, cur_panel->size_relative_to_parent, rect_dimensions.y);
    }
    else
    {
      to_place.x1 = rect.x0 + lerpf(0.0f, cur_panel->size_relative_to_parent, rect_dimensions.x);
    }

    cur_panel->sentinel->rectangle = to_place;
    cur_panel->sentinel->computed_size_in_pixels = rect_get_dimensions(&to_place);

    if (cur_panel->first_child == NULL)
    {
      Instance_Buffer_Element *draw_call = push_struct(&render->render_data, Instance_Buffer_Element);

      draw_call->size  = {0.0f, 0.0f, rect_get_width(&to_place), rect_get_height(&to_place)};
      draw_call->uv    =
      {
        (f32) render->atlas->solid_color_rect.x0,
        (f32) render->atlas->solid_color_rect.y0,
        (f32) render->atlas->solid_color_rect.x1,
        (f32) render->atlas->solid_color_rect.y1,
      };
      draw_call->pos      = V3(to_place.x0, to_place.y0, 0.4f);
      draw_call->color[0] = rgba_from_u8(55, 47, 36, 255);
      draw_call->color[1] = rgba_from_u8(55, 47, 36, 255);
      draw_call->color[2] = rgba_from_u8(55, 47, 36, 255);
      draw_call->color[3] = rgba_from_u8(55, 47, 36, 255);
      draw_call->corner_radius    = global_slider_float * 1000.0f;
      draw_call->border_thickness = 3.0f;
      draw_call->edge_softness    = 0.5f;

      u64 initial_used = temp_arena->used;
      ui_prepare_render(cur_panel->sentinel, to_place);
      temp_arena->used = initial_used;
    }

    arena_pop(panel_stack, sizeof(Panel *));

    if (cur_panel->first_child != NULL)
    {
      first_child = NULL;
      for (Panel *cur_child  = cur_panel->first_child;
           cur_child        != first_child;
           cur_child         = cur_child->next_sibling)
      {
        arena_append(panel_stack, &cur_child, sizeof(Panel *));
        first_child = cur_panel->first_child;
      }
      ui_prepare_render_from_panels(cur_panel, to_place);
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

internal void ui_prepare_render(Widget *widgets, Rect_f32 rect)
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
      {
        first_child = NULL;
        for (Widget *cur_child  = cur_widget->first_child;
             cur_child         != first_child;
             cur_child          = cur_child->next_sibling)
        {
          Widget *child_parent = cur_widget;

          if (cur_child->size_flags & size_flag_to_be_sized_x)
          {
            expect(to_be_sized_x > 0);
            cur_child->computed_size_in_pixels.x = (remaining_width / ((f32) to_be_sized_x));
          }

          if (cur_child->size_flags & size_flag_to_be_sized_y)
          {
            cur_child->computed_size_in_pixels.y = max_height;
          }

          {
            if (cur_child->size_flags & size_flag_relative_to_parent_pos_x)
            {
              cur_child->rectangle.x0 =
                pre_sizing_top_left.x + 
                (cur_child->position_relative_to_parent.x * child_parent->computed_size_in_pixels.x);
            }
            else
            {
              cur_child->rectangle.x0 = cur_top_left.x + cur_child->position_relative_to_parent.x;
            }

            if (cur_child->size_flags & size_flag_relative_to_parent_pos_y)
            {
              cur_child->rectangle.y0 =
                pre_sizing_top_left.y + 
                (cur_child->position_relative_to_parent.y * child_parent->computed_size_in_pixels.y);
             }
            else
            {
              cur_child->rectangle.y0 = cur_top_left.y + cur_child->position_relative_to_parent.x;
            }

            // TODO(antonio): this could mess up text alignment
            cur_child->rectangle.x1 = cur_child->rectangle.x0 + cur_child->computed_size_in_pixels.x;
            cur_child->rectangle.y1 = cur_child->rectangle.y0 + cur_child->computed_size_in_pixels.y;
          }

          cur_top_left.x += cur_child->computed_size_in_pixels.x;

          first_child = cur_widget->first_child;

          if (cur_child->first_child)
          {
            ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
          }
        }
      }

      cur_widget->rectangle.x0 = pre_sizing_top_left.x + cur_widget->position_relative_to_parent.x;
      cur_widget->rectangle.y0 = pre_sizing_top_left.y + cur_widget->position_relative_to_parent.y;

      cur_widget->rectangle.x1 = cur_widget->rectangle.x0 + cur_widget->computed_size_in_pixels.x;
      cur_widget->rectangle.y1 = cur_widget->rectangle.y0 + cur_widget->computed_size_in_pixels.y;

      cur_top_left.x = rect.x0;
      cur_top_left.y = pre_sizing_top_left.y + cur_widget->computed_size_in_pixels.y;
    }
  }

  arena_reset(temp_arena);
  {
    Ring_Buffer widget_queue = ring_buffer_make(temp_arena, structs_in_size(ring_buffer_size, Widget *));
    Widget *first_child = NULL;
    for (Widget *cur_child = widgets->first_child;
         cur_child != first_child;
         cur_child = cur_child->next_sibling)
    {
      ring_buffer_append(&widget_queue, &cur_child, sizeof(Widget *));
      first_child = widgets->first_child;
    }

    b32 keep_hot_key = false;
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

        if (ui_is_key_equal(ui->active_key, cur_widget->key))
        {
          b32 mouse_left_went_up = mouse_left_change && ((ui->cur_frame_mouse_event & mouse_event_lclick) == 0);
          if (mouse_left_went_up)
          {
            if (ui_is_key_equal(ui->hot_key, cur_widget->key))
            {
              ui_add_interaction(cur_widget, 1, 0, &event_value);
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

        if (is_point_in_rect(ui->mouse_pos, cur_widget->rectangle))
        {
          if (ui->active_key == nil_key)
          {
            ui->hot_key = cur_widget->key;
          }
          keep_hot_key = true;
        }

        if ((cur_widget->key == ui->active_key) && (cur_widget->key == ui->hot_key))
        {
          cur_widget->end_background_color[0] = cur_widget->end_background_color[1];
          cur_widget->end_background_color[2] = cur_widget->end_background_color[3];

          cur_widget->end_background_color[1] = saved_background_color[0];
          cur_widget->end_background_color[3] = saved_background_color[2];
        }
      }

      if (cur_widget->widget_flags & widget_flag_dragable)
      {
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
          else
          {
            ui->active_key = nil_key;
          }
        }

        if (is_point_in_rect(ui->mouse_pos, cur_widget->rectangle))
        {
          if (ui->active_key == nil_key)
          {
            ui->hot_key = cur_widget->key;
          }
          keep_hot_key = true;
        }

        if (ui->active_key == cur_widget->key)
        {
          Widget *parent = cur_widget->parent;
          event_value.mouse = V2((ui->mouse_pos.x - parent->rectangle.x0) / parent->computed_size_in_pixels.x, 0.0f);
          ui_add_interaction(cur_widget, 1, 0, &event_value);
        }
      }

      if (cur_widget->widget_flags & widget_flag_draw_background)
      {
        Instance_Buffer_Element *draw_call = push_struct(&render->render_data, Instance_Buffer_Element);

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
          0.5f,
        };

        draw_call->corner_radius    = cur_widget->corner_radius;
        draw_call->border_thickness = cur_widget->border_thickness;
        draw_call->edge_softness    = cur_widget->edge_softness;
      }

      if (cur_widget->widget_flags & widget_flag_draw_text)
      {
        f32 x        = cur_widget->rectangle.x0 + ((f32) ui->text_gutter_dim.x);
        f32 baseline = cur_widget->rectangle.y0 + ui->text_height - ((f32) ui->text_gutter_dim.y);

        set_temp_arena_wait(1);
        render_draw_text(&x, &baseline, cur_widget->text_color, rect, cur_widget->string.str);
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
