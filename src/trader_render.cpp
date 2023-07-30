#include "trader_render.h"

internal b32 render_atlas_initialize(Arena         *arena,
                                     Texture_Atlas *atlas,
                                     File_Buffer   *font_data,
                                     f32           *font_heights,
                                     u32            font_height_count,
                                     u32            bitmap_width,
                                     u32            bitmap_height)
{
  b32 result = true;

  expect((font_heights != NULL) &&
         ((0 < font_height_count) &&
          (font_height_count <= member_array_count(Texture_Atlas, heights))));

  i32 font_count = stbtt_GetNumberOfFonts(font_data->data);
  if (font_count > 0) 
  {
    stbtt_fontinfo font_info = {};
    i32 font_offset = stbtt_GetFontOffsetForIndex(font_data->data, 0);

    // NOTE(antonio): fallback font
    if (!stbtt_InitFont(&font_info, font_data->data, font_offset))
    {
      expect_message(default_font.data != NULL, "expected a default font at least...");

      font_data = &default_font;
      stbtt_InitFont(&font_info, font_data->data, 0);
    }

    // NOTE(antonio): the bitmap only encodes alpha
    u8 *bitmap_data = (u8 *) arena_push(arena, bitmap_width * bitmap_height);

    if (bitmap_data == NULL)
    {
      expect_message(false, "expected arena allocation to succeed");
    }

    stbtt_pack_context pack_context;
    stbtt_PackBegin(&pack_context, bitmap_data, bitmap_width, bitmap_height, 0, 1, NULL);
    {
      u32 code_point_count = (ending_code_point - starting_code_point) + 1;

      // NOTE(antonio): the extra is the solid color glyph
      u32               rect_count        = (code_point_count * font_height_count) + 1;
      u32               packed_char_count = rect_count - 1;

      stbtt_packedchar *packed_chars      = push_array_zero(arena, stbtt_packedchar, packed_char_count);
      stbrp_rect       *rects             = push_array(arena, stbrp_rect, rect_count);

      stbtt_pack_range pack_ranges[4] = {};

      stbrp_rect       *rects_being_packed        = rects;
      stbtt_packedchar *packed_chars_being_packed = packed_chars;

      for (u32 pack_range_index = 0;
           pack_range_index < font_height_count;
           ++pack_range_index)
      {
        pack_ranges[pack_range_index].font_size                        = font_heights[pack_range_index];
        pack_ranges[pack_range_index].first_unicode_codepoint_in_range = starting_code_point;
        pack_ranges[pack_range_index].array_of_unicode_codepoints      = NULL;
        pack_ranges[pack_range_index].num_chars                        = code_point_count;
        pack_ranges[pack_range_index].chardata_for_range               = packed_chars_being_packed;
        
        if (font_heights[pack_range_index] < 30.0f) stbtt_PackSetOversampling(&pack_context, 2, 2);
        else                                        stbtt_PackSetOversampling(&pack_context, 1, 1);

        stbtt_PackFontRangesGatherRects(&pack_context,
                                        &font_info,
                                        &pack_ranges[pack_range_index],
                                        1,
                                        rects_being_packed);

        rects_being_packed        += code_point_count;
        packed_chars_being_packed += code_point_count;
      }

      stbrp_rect *residue_rect = rects_being_packed;
      {
        residue_rect->w = 1;
        residue_rect->h = 1;
      }

      stbtt_PackFontRangesPackRects(&pack_context, rects, rect_count);
      i32 stb_result = stbtt_PackFontRangesRenderIntoRects(&pack_context,
                                                           &font_info,
                                                           pack_ranges,
                                                           font_height_count,
                                                           rects);
      result = (b32) stb_result;

      // render the rest
      bitmap_data[(residue_rect->y * bitmap_width) + residue_rect->x] = 0xff;

      pop_array(arena, stbrp_rect, rect_count);
      if (result)
      {
        atlas->bitmap.alpha  = bitmap_data;
        atlas->bitmap.width  = (f32) bitmap_width;
        atlas->bitmap.height = (f32) bitmap_height;

        copy_struct(&atlas->font_info, &font_info);

        for (u64 packed_char_index = 0;
             packed_char_index < packed_char_count;
             ++packed_char_index)
        {
          // stbtt_packedchar *packed_char = packed_chars + packed_char_index;
          // packed_char->yoff *= -1.0f;
        }
        atlas->char_data = packed_chars;


        copy_array(atlas->heights, font_heights, font_height_count);

        for (u32 count_index = 0;
             count_index < font_height_count;
             ++count_index)
        {
          atlas->char_data_set_counts[count_index] = code_point_count;
        }

        atlas->height_count    = font_height_count;

        atlas->solid_color_rect.x0 = (i16) residue_rect->x; 
        atlas->solid_color_rect.y0 = (i16) residue_rect->y;
        atlas->solid_color_rect.x1 = (i16) (atlas->solid_color_rect.x0 + residue_rect->w);
        atlas->solid_color_rect.y1 = (i16) (atlas->solid_color_rect.y0 + residue_rect->h);
      }
      else
      {
        pop_array(arena, stbtt_packedchar, packed_char_count);
        arena_pop(arena, bitmap_width * bitmap_height);
      }
    }
    stbtt_PackEnd(&pack_context);
  }

  return(result);
}

internal Common_Render_Context *render_get_common_context(void)
{
  Common_Render_Context *common = (Common_Render_Context *) render_get_context();
  return(common);
}

internal void render_set_client_rect(Rect_f32 new_rect)
{
  Common_Render_Context *render = render_get_common_context();
  render->client_rect = new_rect;
}

internal Rect_f32 render_get_client_rect(void)
{
  Common_Render_Context *render = render_get_common_context();
  return(render->client_rect);
}

internal Vertex_Buffer_Element *render_push_triangles(u64 triangle_count)
{
  Vertex_Buffer_Element *data_to_fill =
    push_array(&render_get_common_context()->triangle_render_data, Vertex_Buffer_Element, 3 * triangle_count);
  return(data_to_fill);
}

internal void render_data_to_lines(V2_f32 *points, u64 point_count)
{
  Vertex_Buffer_Element *vertices = render_push_triangles((point_count - 1) * 2);
  Vertex_Buffer_Element *cur_vert = vertices;

  Common_Render_Context *common = render_get_common_context();
  Rect_f32 solid_color_rect = render_get_solid_color_rect();
    /*{
    (f32) common->atlas->solid_color_rect.x0,
    (f32) common->atlas->solid_color_rect.y0,
    (f32) common->atlas->solid_color_rect.x1,
    (f32) common->atlas->solid_color_rect.y1,
  };*/

  f32 to_next_pixel = (1.0f / (2.0f * common->vertex_render_dimensions.x));

  for (i64 cur_pair = 0;
       cur_pair < (i64) (point_count - 1);
       ++cur_pair)
  {
    // TODO(antonio): check if pairs are the same...
    V2_f32 start = points[cur_pair];
    V2_f32 end   = points[cur_pair + 1];

    if (squared_length(start - end) >= 0.00005f)
    {
      V2_f32 delta  = V2(end.x - start.x, end.y - start.y);

      V2_f32 normal = V2(-delta.y, delta.x);
      if (dot(normal, up_v2) < 0.0f) {
        normal = -1.0f * normal;
      }

      normal = to_next_pixel * normalize(normal);

      V2_f32 tl = start + normal + (0.5f * V2(-to_next_pixel, -to_next_pixel));
      V2_f32 bl = start - normal + (0.5f * V2(-to_next_pixel,  to_next_pixel));
      V2_f32 tr = end   + normal + (0.5f * V2( to_next_pixel, -to_next_pixel));
      V2_f32 br = end   - normal + (0.5f * V2( to_next_pixel,  to_next_pixel));

      // TL
      *cur_vert++ =
      {
        V4(tl.x, tl.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x0, solid_color_rect.y0)
      };

      // BL
      *cur_vert++ =
      {
        V4(bl.x, bl.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x0, solid_color_rect.y1)
      };

      // TR
      *cur_vert++ =
      {
        V4(tr.x, tr.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x1, solid_color_rect.y0)
      };

      // BL
      *cur_vert++ =
      {
        V4(bl.x, bl.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x0, solid_color_rect.y1)
      };

      // TR
      *cur_vert++ =
      {
        V4(tr.x, tr.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x1, solid_color_rect.y0)
      };

      // BR
      *cur_vert++ =
      {
        V4(br.x, br.y, 0.5f, 1.0f),
        rgba(1.0f, 1.0f, 1.0, 1.0f),
        V2(solid_color_rect.x1, solid_color_rect.y1)
      };
    }
  }
}

internal void render_get_text_dimensions(f32 *x, f32 *y, Rect_f32 bounds, String_Const_utf8 string, i64 up_to)
{
  expect(x != NULL);
  expect(y != NULL);

  expect(bounds.x0 <= bounds.x1);
  expect(bounds.y0 <= bounds.y1);

  // TODO(antonio): kind of a hack
  expect(is_between_inclusive(-1, up_to, (i64) string.size));

  Common_Render_Context   *render_context  = render_get_common_context();
  Texture_Atlas           *atlas           = render_context->atlas;

  V2_f32 cur_pos = V2(*x, *y);
  f32 font_scale = stbtt_ScaleForPixelHeight(&atlas->font_info, atlas->heights[0]);

  for (i64 text_index = 0;
       (text_index < (i64) string.size) && (text_index <= up_to);
       ++text_index)
  {
    // TODO(antonio): deal with new lines more gracefully
    if (is_newline(string.str[text_index]))
    {
      continue;
    }
    else
    {
      stbtt_packedchar *cur_packed_char = atlas->char_data + (string.str[text_index] - starting_code_point);

      f32 kern_advance = 0.0f;
      kern_advance = font_scale *
        stbtt_GetCodepointKernAdvance(&atlas->font_info,
                                      string.str[text_index],
                                      string.str[text_index + 1]);

      cur_pos.x += kern_advance + cur_packed_char->xadvance;
    }
  }

  *x = cur_pos.x;
  *y = cur_pos.y + font_scale;
}

internal void render_draw_text(Arena    *render_arena,
                               f32      *baseline_x,
                               f32      *baseline_y,
                               RGBA_f32  color,
                               Rect_f32  bounds,
                               utf8     *format, ...)
{
  expect(render_arena != NULL);

  expect(baseline_x != NULL);
  expect(baseline_y != NULL);

  expect(bounds.x0 <= bounds.x1);
  expect(bounds.y0 <= bounds.y1);

  Arena *temp_arena = get_temp_arena();

  u64 sprinted_text_cap = 512;
  String_utf8 sprinted_text =
  {
    (utf8 *) arena_push(temp_arena, sprinted_text_cap),
    0,
    sprinted_text_cap
  };

  va_list args;
  va_start(args, format);
  sprinted_text.size = stbsp_vsnprintf((char *) sprinted_text.str, (i32) sprinted_text.cap, (char *) format, args);
  va_end(args);

  Common_Render_Context   *render_context  = render_get_common_context();
  Texture_Atlas           *atlas           = render_context->atlas;
  Instance_Buffer_Element *render_elements = push_array_zero(render_arena,
                                                             Instance_Buffer_Element,
                                                             sprinted_text.size);

  f32 font_scale = stbtt_ScaleForPixelHeight(&atlas->font_info, atlas->heights[0]);

  V2_f32 cur_pos = V2(*baseline_x, *baseline_y);
  for (u64 text_index = 0;
       (sprinted_text.str[text_index] != '\0') && (text_index < sprinted_text.size);
       ++text_index)
  {
    // TODO(antonio): deal with new lines more gracefully
    if (is_newline(sprinted_text.str[text_index]))
    {
      continue;
    }
    else
    {
      Instance_Buffer_Element *cur_element     = render_elements  +  text_index;
      stbtt_packedchar        *cur_packed_char = atlas->char_data + (sprinted_text.str[text_index] - starting_code_point);

      f32 kern_advance = 0.0f;
      if (text_index < (sprinted_text.size - 1))
      {
        kern_advance = font_scale *
          stbtt_GetCodepointKernAdvance(&atlas->font_info,
                                        sprinted_text.str[text_index],
                                        sprinted_text.str[text_index + 1]);
      }

      if (is_between_inclusive(bounds.x0, cur_pos.x + cur_packed_char->xadvance, bounds.x1) &&
          is_between_inclusive(bounds.y0, cur_pos.y, bounds.y1))
      {
        cur_element->pos = 
        {
          cur_pos.x + cur_packed_char->xoff,
          cur_pos.y + cur_packed_char->yoff,
          0.6f
        };

        cur_element->size = 
        {
          0.0f,
          0.0f,
          (f32) (cur_packed_char->xoff2 - cur_packed_char->xoff),
          (f32) (cur_packed_char->yoff2 - cur_packed_char->yoff)
        };

        cur_element->uv = 
        {
          (f32) cur_packed_char->x0,
          (f32) cur_packed_char->y0,
          (f32) cur_packed_char->x1,
          (f32) cur_packed_char->y1,
        };

        cur_element->color[0] = color;
        cur_element->color[1] = color;
        cur_element->color[2] = color;
        cur_element->color[3] = color;

        cur_element->edge_softness = 0.0f;

        cur_pos.x += kern_advance + cur_packed_char->xadvance;
      }
      else
      {
        break;
      }
    }
  }

  *baseline_x = cur_pos.x;
  *baseline_y = cur_pos.y;
}

internal Rect_f32 render_get_solid_color_rect(void)
{
  Texture_Atlas *atlas = render_get_common_context()->atlas;

  Rect_f32 solid_color_rect;

  solid_color_rect.x0 = ((f32) atlas->solid_color_rect.x0) + 0.5f;
  solid_color_rect.y0 = ((f32) atlas->solid_color_rect.y0) + 0.5f;
  solid_color_rect.x1 = ((f32) atlas->solid_color_rect.x1) - 0.5f;
  solid_color_rect.y1 = ((f32) atlas->solid_color_rect.y1) - 0.5f;

  return(solid_color_rect);
}

internal void render_push_line_instance(V2_f32 line_start, f32 length, f32 dir_x, f32 dir_y, RGBA_f32 color)
{
  expect(xorb(dir_x != 0.0f, dir_y != 0.0f));

  Common_Render_Context *common_context =  render_get_common_context();
  Arena                 *instance_data  = &common_context->render_data;

  Instance_Buffer_Element *draw = push_struct_zero(instance_data, Instance_Buffer_Element);

  V2_f32 line_sizes = V2(dir_x ? dir_x * length : 1.0f, dir_y ? dir_y * length : 1.0f);
  draw->size.p1  = line_sizes;

  draw->pos      = V3(line_start.x + 0.5f, line_start.y + 0.5f, 0.5f);

  draw->color[0] = color;
  draw->color[1] = color;
  draw->color[2] = color;
  draw->color[3] = color;

  draw->uv       = render_get_solid_color_rect();
}

internal i64 render_get_packed_char_start(f32 font_height)
{
  Texture_Atlas *atlas = render_get_common_context()->atlas;

  i64 result = 0;
  b32 found = false;

  for (u64 font_height_index = 0;
       font_height_index < array_count(atlas->heights);
       ++font_height_index)
  {
    f32 cur_height = atlas->heights[font_height_index];

    if (approx_equal_f32(cur_height, font_height))
    {
      found = true;
      break;
    }
    else
    {
      result += atlas->char_data_set_counts[font_height_index];
    }
  }

  return(found ? result : -1);
}

