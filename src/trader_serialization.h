#if !defined(TRADER_SERIALIZATION_H)

// NOTE(antonio): unicode byte marker
global_const u8  serial_byte_marker[2] = {0xfe, 0xff};

global_const u32 serial_ui_cur_version               = 1;
global_const u32 serial_ui_earliest_accepted_version = 1;

struct Serialization
{
  u8  endian_marker[2];
  u16 serial_kind;
  u32 version;
  u64 length;
  u8  data[1];
};

internal Serialization *serial_ui_serialize(u8 *data, u64 data_length, u8 *into, u64 into_length);
internal void           serial_ui_deserialize(Serialization *serialized);

// NOTE(antonio): implementation
internal Serialization *serial_ui_serialize(u8 *data, u64 data_length, u8 *into, u64 into_length)
{
  unused(data);
  unused(data_length);
  unused(into);
  unused(into_length);

  Serialization *constructing = (Serialization *) into;

  Serialization *res = constructing;
  return(res);
}

#endif
