#ifndef TRADER_FONT_H

#define stbtt_uint8  u8
#define stbtt_int8   i8
#define stbtt_uint16 u16
#define stbtt_int16  i16
#define stbtt_uint32 u32
#define stbtt_int32  i32

struct Font_Data {
  Font_Bitmap       bitmap;
  stbtt_packedchar *char_data;

  f32               heights[4];
  u32               char_data_set_counts[4];
  u32               height_count;
};

#define TRADER_FONT_H
#endif
