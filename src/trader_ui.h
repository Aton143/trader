#ifndef TRADER_UI_H

typedef u64 Widget_Flag;
enum
{
  widget_flag_none               = (0LL),

  widget_flag_clickable          = (1LL << 0),
  widget_flag_draggable          = (1LL << 1),

  widget_flag_view_scroll        = (1LL << 2),
  widget_flag_keyboard_focusable = (1LL << 3),

  widget_flag_disabled           = (1LL << 4),

  widget_flag_floating_x         = (1LL << 5),
  widget_flag_floating_y         = (1LL << 6),

  widget_flag_allow_overflow_x   = (1LL << 7),
  widget_flag_allow_overflow_y   = (1LL << 8),

  widget_flag_draw_text          = (1LL << 9),
  widget_flag_truncate_text      = (1LL << 10),

  // NOTE(antonio): no need? should be data-driven?
  widget_flag_draw_border        = (1LL << 11),

  widget_flag_draw_background    = (1LL << 12),
  widget_flag_draw_dropshadow    = (1LL << 13),

  // TODO(antonio): ??
  widget_flag_clip               = (1LL << 14),

  widget_flag_hot_animation      = (1LL << 15),
  widget_flag_active_animation   = (1LL << 16),

  widget_flag_smooth_animation_x = (1LL << 17),
  widget_flag_smooth_animation_y = (1LL << 18),

  widget_flag_arbitrary_draw     = (1LL << 19),
  widget_flag_border_draggable   = (1LL << 20),
  widget_flag_top_level          = (1LL << 21),
};

typedef u64 Widget_Size_Flag;
enum
{
  size_flag_none                     = (0LL),

  size_flag_content_size_x           = (1LL << 0),
  size_flag_content_size_y           = (1LL << 1),
  size_flag_text_content             = (size_flag_content_size_x | size_flag_content_size_y),

  size_flag_fill_rest_of_axis_x      = (1LL << 2),
  size_flag_fill_rest_of_axis_y      = (1LL << 3),

  size_flag_copy_parent_size_x       = (1LL << 4),
  size_flag_copy_parent_size_y       = (1LL << 5),

  size_flag_to_be_sized_x            = (1LL << 6),
  size_flag_to_be_sized_y            = (1LL << 7),

  size_flag_given_size_x             = (1LL << 8),
  size_flag_given_size_y             = (1LL << 9),

  size_flag_absolute_pos_x           = (1LL << 10),
  size_flag_absolute_pos_y           = (1LL << 11),

  size_flag_relative_to_parent_pos_x = (1LL << 10),
  size_flag_relative_to_parent_pos_y = (1LL << 11),

  size_flag_advancer_x               = (1LL << 12),
  size_flag_advancer_y               = (1LL << 13),
};

typedef u32 Mouse_Area;
enum
{
  mouse_area_none,
  mouse_area_out_client,
  mouse_area_in_client,
  mouse_area_other,
  mouse_area_count
};

typedef u32 Mouse_Event;
enum
{
  mouse_event_none   = 0,
  mouse_event_lclick = 1,
  mouse_event_rclick = (1 << 1),
  mouse_event_mclick = (1 << 2),

  // TODO(antonio): other types?
};

enum
{
  key_event_none = 0,
  key_event_tab,
  key_event_left_arrow,
  key_event_right_arrow,
  key_event_up_arrow,
  key_event_down_arrow,
  key_event_page_up,
  key_event_page_down,
  key_event_home,
  key_event_end,
  key_event_insert,
  key_event_delete,
  key_event_backspace,
  key_event_space,
  key_event_enter,
  key_event_escape,
  key_event_apostrophe,
  key_event_comma,
  key_event_minus,
  key_event_period,
  key_event_slash,
  key_event_semicolon,
  key_event_equal,
  key_event_left_bracket,
  key_event_backslash,
  key_event_right_bracket,
  key_event_grave_accent,
  key_event_caps_lock,
  key_event_scroll_lock,
  key_event_num_lock,
  key_event_print_screen,
  key_event_pause,
  key_event_keypad_0,
  key_event_keypad_1,
  key_event_keypad_2,
  key_event_keypad_3,
  key_event_keypad_4,
  key_event_keypad_5,
  key_event_keypad_6,
  key_event_keypad_7,
  key_event_keypad_8,
  key_event_keypad_9,
  key_event_keypad_decimal,
  key_event_keypad_divide,
  key_event_keypad_multiply,
  key_event_keypad_subtract,
  key_event_keypad_add,
  key_event_keypad_enter,
  key_event_left_shift,
  key_event_left_ctrl,
  key_event_left_alt,
  key_event_left_super,
  key_event_right_shift,
  key_event_right_ctrl,
  key_event_right_alt,
  key_event_right_super,
  key_event_menu,
  key_event_0,
  key_event_1,
  key_event_2,
  key_event_3,
  key_event_4,
  key_event_5,
  key_event_6,
  key_event_7,
  key_event_8,
  key_event_9,
  key_event_a,
  key_event_b,
  key_event_c,
  key_event_d,
  key_event_e,
  key_event_f,
  key_event_g,
  key_event_h,
  key_event_i,
  key_event_j,
  key_event_k,
  key_event_l,
  key_event_m,
  key_event_n,
  key_event_o,
  key_event_p,
  key_event_q,
  key_event_r,
  key_event_s,
  key_event_t,
  key_event_u,
  key_event_v,
  key_event_w,
  key_event_x,
  key_event_y,
  key_event_z,
  key_event_f1,
  key_event_f2,
  key_event_f3,
  key_event_f4,
  key_event_f5,
  key_event_f6,
  key_event_f7,
  key_event_f8,
  key_event_f9,
  key_event_f10,
  key_event_f11,
  key_event_f12,

  key_mod_event_control,
  key_mod_event_shift,
  key_mod_event_alt,
  key_mod_event_super,

  key_event_count,
};

struct Mod_Keys
{
  b8 control;
  b8 shift;
  b8 alt;
  b8 super;
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

  RGBA_f32          text_color;

  // NOTE(antonio): 
  //   0 - top-left
  //   1 - bottom-left
  //   2 - top-right
  //   3 - bottom-right
  RGBA_f32          background_color[4];
  RGBA_f32          end_background_color[4];

  f32               font_height;

  f32               corner_radius;
  f32               edge_softness;
  f32               border_thickness;

  // NOTE(antonio): computed every frame
  V2_f32            position_relative_to_parent;
  V2_f32            extra_sizing;

  V2_f32            computed_size_in_pixels;
  Rect_f32          rectangle;

  // NOTE(antonio): persistent data
  f32               hot_time;
  f32               active_time;
  f32               time_alive;
};

struct Widget_Parameters
{
  V2_f32             sizing           = {1.0f, 1.0f};
  V2_f32             pos              = {0.0f, 0.0f};
  f32                corner_radius    = 0.0f;
  f32                edge_softness    = 1.0f;
  f32                border_thickness = 0.0f;
  void              *data             = NULL;
  u64                data_size        = 0;
};

union UI_Event_Value
{
  V2_f32 mouse;
};

struct UI_Interaction
{
  UI_Key         key;
  i32            frames_left;
  u32            event;
  UI_Event_Value value;

#if !SHIP_MODE
  // TODO(antonio): remove - for debugging
  UI_Event_Value value2;
  u64            start_frame;
#endif
};

struct Persistent_Widget_Data
{
  UI_Key   key;
  RGBA_f32 background_color[4];
};

struct Panel;
struct UI_Context
{
  Panel                  *current_panel_parent;
  Panel                  *panels_start;
  Panel                  *panel_free_list_head;
  u32                     panel_memory_size;
  u32                     panel_count;

  // NOTE(antonio): only one can be active at any given time
  UI_Key                  hot_key;    // NOTE(antonio): about to interact
  UI_Key                  active_key; // NOTE(antonio): interacting

  Widget                 *widget_memory;
  u64                     widget_memory_size;

  Widget                 *allocated_widgets;
  Widget                 *widget_free_list_head;

  Arena                  *string_pool;

  u32                     max_widget_count;
  u32                     current_widget_count;

  V2_i16                  text_gutter_dim;

  Mouse_Area              mouse_area;

  Mouse_Event             prev_frame_mouse_event;
  Mouse_Event             cur_frame_mouse_event;

  V2_f32                  mouse_pos;
  V2_f32                  mouse_delta;
  V2_f32                  mouse_wheel_delta;

  f32                     text_height;
  RGBA_f32                text_color;
  RGBA_f32                background_color[4];

  UI_Interaction          interactions[4];

  Rect_f32                canvas_viewport;

  // TODO(antonio): when does this get cleared?
  Persistent_Widget_Data  persistent_data[4];

  Mod_Keys                mod_keys;
  b8                      key_events[key_event_count];
  b8                      keep_hot_key;
};

enum
{
  axis_split_none,
  axis_split_horizontal,
  axis_split_vertical,
};
typedef u32 Axis_Split;

struct Panel
{
  Panel      *first_child;
  Panel      *last_child;

  Panel      *next_sibling;
  Panel      *previous_sibling;

  Panel      *parent;

  Axis_Split  split;
  f32         size_relative_to_parent;

  String_Const_utf8 string;

  Widget     *sentinel;
  Widget     *current_parent;

  f32         sizing_left;
};

#include "trader_platform.h"
#include "trader_render.h"

global_const UI_Key             nil_key                  = 0;
global_const f32                default_text_height      = 24.0f;
global_const V2_i16             default_text_gutter_dim  = {2, 4};
global_const RGBA_f32           default_text_color       = rgba(1.0f, 1.0f, 1.0f, 1.0);
global_const u32                default_widget_count     = 4096;
global_const u64                default_string_pool_size = kb(16);
global_const V2_f32             default_widget_sizing    = V2(1.0f, 1.0f);
global_const u32                default_panel_count      = 64;
global_const String_Const_utf8  default_panel_string     = string_literal_init_type("default panel string", utf8);

global Persistent_Widget_Data default_persistent_data = {};
global_const RGBA_f32 default_background_color[4] =
{
  rgba(0.0f, 0.0f, 0.0f, 1.0),
  rgba(0.0f, 0.0f, 0.0f, 1.0),
  rgba(0.0f, 0.0f, 0.0f, 1.0),
  rgba(0.0f, 0.0f, 0.0f, 1.0)
};

internal inline UI_Context *ui_get_context(void);

internal inline void ui_add_interaction(Widget *cur_widget, i32 frames_left, u32 event, UI_Event_Value *event_value);

internal inline void ui_add_key_event(Key_Event event, b32 is_down);

internal void ui_initialize(UI_Context *ui);
internal void ui_initialize_frame(void);

internal void ui_prepare_render(Widget *widgets, Rect_f32 rect);

internal void ui_update_persistent_data(Persistent_Widget_Data *data);
internal Persistent_Widget_Data *ui_search_persistent_data(Widget *widget);

internal void ui_set_text_height(f32 height);

internal void ui_push_text_color(f32 r, f32 g, f32 b, f32 a);
internal void ui_pop_text_color(void);

internal inline void ui_push_background_color(f32 r, f32 g, f32 b, f32 a);
internal inline void ui_push_background_color(RGBA_f32 color);
internal void ui_push_background_color();

internal void ui_push_parent(Widget *widget);
internal void ui_pop_parent(void);

// NOTE(antonio): widgets
internal void ui_do_string(String_Const_utf8 string);
internal void ui_do_formatted_string(char *format, ...);

internal b32  ui_do_button(String_Const_utf8 string);

internal void ui_do_slider_f32(String_Const_utf8 string, f32 *in_out_value, f32 minimum, f32 maximum);

internal void ui_canvas(String_Const_utf8 string, V2_f32 size = {400.0f, 400.0f});

internal UI_Key ui_make_key(String_Const_utf8 string);
internal b32 ui_is_key_equal(UI_Key a, UI_Key b);

internal Widget *ui_make_sentinel_widget(void);
internal void    ui_make_widget(Widget_Flag        widget_flags,
                                Widget_Size_Flag   size_flags,
                                String_Const_utf8  string,
                                Widget_Parameters *params);

internal void ui_make_widget(Widget_Flag        widget_flags,
                             Widget_Size_Flag   size_flags,
                             String_Const_utf8  string,
                             V2_f32             sizing           = {1.0f, 1.0f},
                             V2_f32             pos              = {0.0f, 0.0f},
                             f32                corner_radius    = 0.0f,
                             f32                edge_softness    = 0.0f,
                             f32                border_thickness = 0.0f,
                             void              *data             = NULL,
                             u64                data_size        = 0);
// NOTE(antonio): panels
internal inline Panel *ui_get_sentinel_panel(void);

internal inline void ui_push_panel_parent(Panel *panel);
internal inline void ui_pop_panel_parent(void);

internal inline f32  ui_get_remaining_sizing_from_implicit_panel_parent(void);

internal Panel *ui_make_panel(Axis_Split         split,
                              f32               *size_relative_to_parent,
                              String_Const_utf8  string = {},
                              Panel             *from   = NULL);
// returns first child
internal Panel *ui_make_panels(Axis_Split         split,
                               f32               *sizes,
                               String_Const_utf8 *strings,
                               u32                count,
                               Panel             *to_split = NULL);

internal void   ui_prepare_render_from_panels(Panel *panel, Rect_f32 rect);
#define TRADER_UI_H
#endif
