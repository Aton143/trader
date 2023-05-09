#ifndef TRADER_UI_H

typedef u64 Widget_Flag;
enum
{
  widget_flag_none               = (0LL),

  widget_flag_clickable          = (1LL << 0),

  widget_flag_view_scroll        = (1LL << 1),
  widget_flag_keyboard_focusable = (1LL << 2),

  widget_flag_disabled           = (1LL << 3),

  widget_flag_floating_x         = (1LL << 4),
  widget_flag_floating_y         = (1LL << 5),

  widget_flag_allow_overflow_x   = (1LL << 6),
  widget_flag_allow_overflow_y   = (1LL << 7),

  widget_flag_draw_text          = (1LL << 8),
  widget_flag_truncate_text      = (1LL << 9),

  widget_flag_draw_border        = (1LL << 10),
  widget_flag_draw_background    = (1LL << 11),
  widget_flag_draw_dropshadow    = (1LL << 12),

  widget_flag_clip               = (1LL << 13),

  widget_flag_hot_animation      = (1LL << 14),
  widget_flag_active_animation   = (1LL << 15),

  widget_flag_smooth_animation_x = (1LL << 16),
  widget_flag_smooth_animation_y = (1LL << 17),
};

typedef u64 Widget_Size_Flag;
enum
{
  size_flag_none                = (0LL),

  size_flag_content_size_x      = (1LL << 0),
  size_flag_content_size_y      = (1LL << 1),
  size_flag_text_content        = (size_flag_content_size_x | size_flag_content_size_y),

  size_flag_fill_rest_of_axis_x = (1LL << 2),
  size_flag_fill_rest_of_axis_y = (1LL << 3),

  size_flag_copy_parent_size_x  = (1LL << 4),
  size_flag_copy_parent_size_y  = (1LL << 5),

  size_flag_to_be_sized_x       = (1LL << 30),
  size_flag_to_be_sized_y       = (1LL << 31),
};

typedef u64 UI_Key;
struct Widget
{
  Widget          *first_child;
  Widget          *last_child;

  Widget          *next_sibling;
  Widget          *previous_sibling;

  Widget          *parent;

  Widget          *hash_next;
  Widget          *hash_previous;

  UI_Key           key;
  u64              last_frame_touched_index;

  Widget_Flag      widget_flags;
  Widget_Size_Flag size_flags;

  String_Const_utf8 string;

  // NOTE(antonio): computed every frame
  V2_f32            computed_position_relative_to_parent;
  V2_f32            computed_size_in_pixels;
  Rect_f32          rectangle;

  // NOTE(antonio): persistent data
  f32               hot_time;
  f32               active_time;
};

global_const Rect_i16 default_text_gutter_dim  = {10, 5};
global_const RGBA_f32 default_text_color       = rgba(1.0f, 1.0f, 1.0f, 1.0);
global_const RGBA_f32 default_background_color = rgba(1.0f, 1.0f, 1.0f, 1.0);
global_const u64      default_widget_count     = 4096;

struct UI_Context
{
  UI_Key    hot_key;
  UI_Key    active_key;

  Widget   *current_parent;

  Widget   *widget_memory;
  u64       widget_memory_size;

  Widget   *allocated_widgets;
  Widget   *widget_free_list_head;

  u32       max_widget_count;
  u32       current_widget_count;

  Rect_i16  text_gutter_dim;

  V2_f32    drag_delta;

  RGBA_f32  text_color;
  RGBA_f32  background_color;

  b8        clicked;
  b8        double_clicked;
  b8        right_clicked;
  b8        pressed;
  b8        released;
  b8        dragging;
  b8        hovering;
};

internal UI_Context *ui_get_context(void);
internal void ui_initialize_frame(void);

internal void ui_set_text_color(f32 r, f32 g, f32 b, f32 a);
internal void ui_set_background_color(f32 r, f32 g, f32 b, f32 a);

internal void ui_push_parent(Widget *widget);
internal void ui_pop_parent(void);

internal UI_Key ui_make_key(String_Const_utf8 string);
internal b32 ui_is_key_equal(UI_Key a, UI_Key b);

#define TRADER_UI_H
#endif