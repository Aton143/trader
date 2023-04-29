#ifndef TRADER_BASE_DEFINES_H

#if defined(_MSC_VER)

#define COMPILER_CL 1

#if defined(_WIN32)
#  define OS_WINDOWS 1
#else
#  error This compiler/platform combination is not supported
#endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# elif defined(_M_ARM)
#  define ARCH_ARM32 1
# else
#  error Architecture is not supported
# endif

#else

#  error This compiler is not supported yet

#endif

// NOTE(antonio): convert all the others to 0

#if !defined(ARCH_32BIT)
#  define ARCH_32BIT 0
#endif
#if !defined(ARCH_64BIT)
#  define ARCH_64BIT 0
#endif
#if !defined(ARCH_X64)
#  define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#  define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#  define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#  define ARCH_ARM32 0
#endif
#if !defined(COMPILER_CL)
#  define COMPILER_CL 0
#endif
#if !defined(COMPILER_GCC)
#  define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
#  define COMPILER_CLANG 0
#endif
#if !defined(OS_WINDOWS)
#  define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#  define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#  define OS_MAC 0
#endif

#if COMPILER_CL
#  define COMPILER_NAME "cl"
#elif COMPILER_CLANG
#  define COMPILER_NAME "clang"
#elif COMPILER_GCC
#  define COMPILER_NAME "gcc"
#else
#  error no name for this compiler
#endif

#if OS_WINDOWS
#  define OS_NAME "win"
#elif OS_LINUX
#  define OS_NAME "linux"
#elif OS_MAC
#  define OS_NAME "mac"
#else
#  error no name for this operating system
#endif

#if COMPILER_CL
# if _MSC_VER <= 1800
#  define snprintf _snprintf
# endif

# if (_MSC_VER <= 1500)
#  define JUST_GUESS_INTS 1
# endif
#endif

#if ARCH_32BIT && OS_WINDOWS
# define CALL_CONVENTION __stdcall
#else
# define CALL_CONVENTION
#endif

#if defined(JUST_GUESS_INTS)
typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

#else
#include <stdint.h> 

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uintptr_t ptr_val;
#endif

typedef float    f32;
typedef double   f64;

typedef i8       b8;
typedef i32      b32;
typedef i64      b64;

#define unused(Variable) (void) (Variable);
#define macro_do(statement) do { statement; } while (0)

#define assert_break(m) (*((volatile i32 *) 0) = 0xfee1)
#define assert_always(c) macro_do( if (!(c)) { assert_break(c); } )
#define assert_message_always(m) assert_break(m)

#if !SHIP_MODE
#define assert(c) assert_always(c)
#define assert_message(m) assert_message_always(m)
#else
#define assert(c)
#define assert_message(m)
#endif

#define concat_(a,b) a##b
#define concat(a,b) concat_(a,b)

#define stringify_(a) #a
#define stringify(a) stringify_(a)

#ifdef swap
#undef swap
#endif

#define swap(t,a,b) macro_do(t concat(hidden_temp_,__LINE__) = a; a = b; b = concat(hidden_temp_,__LINE__))

#define assert_implies(a,b) assert(!(a) || (b))
#define invalid_path        assert_message("invalid path")
#define not_implemented     assert_message("not implemented")

#define  b(x)  (x)
#define kb(x) ((x)      << 10)
#define mb(x) ((x)      << 20)
#define gb(x) ((x)      << 30)
#define tb(x) (((u64)x) << 40)

#define thousand(x) ((x) * 1000)
#define million(x)  ((x) * 1000000)
#define billion(x)  ((x) * 1000000000)

#define has_flag(fi,fl)     (((fi) & (fl)) !=  0)
#define has_all_flag(fi,fl) (((fi) & (fl)) == (fl))
#define add_flag(fi,fl)     ( (fi)         |= (fl))
#define rem_flag(fi,fl)     ( (fi)         &= (~(fl)))

#ifdef max
#undef max
#endif
#define max(a,b) (((a)>(b))?(a):(b))

#ifdef min
#undef min
#endif
#define min(a,b) (((a)<(b))?(a):(b))

#define clamp_top(a,b)    min(a,b)
#define clamp_bottom(a,b) max(a,b)
#define clamp_(a,x,b)     ((a>x)?a:((b<x)?b:x))
#define clamp(a,x,b)      clamp_((a),(x),(b))

#define internal      static
#define local_persist static
#define global        static
#define local_const   static const
#define global_const  static const
#define external      extern "C"
#define unimplemented

#define member_offset(st, m) ((size_t)((u8 *)&((st *)0)->m - (u8 *) 0))
#define member_size(type, member) sizeof(((type *)0)->member)
#define member_array_count(type, member) (member_size(type, member) / sizeof(*((type *)0)->member))

#define array_count(a) ((sizeof(a))/(sizeof(*a)))
#define array_initr(a) {(a), array_count(a)}

#define line_number_as_string stringify(__LINE__)
#define file_name_line_number __FILE__ ":" line_number_as_string ":"

#define is_between_exclusive(a,x,b) (((a) <  (x)) && ((x) < (b)))
#define is_between_inclusive(a,x,b) (((a) <= (x)) && ((x) <= (b)))

global_const u8  max_u8      =  0xFF;
global_const u16 max_u16     =  0xFFFF;
global_const u32 max_u32     =  0xFFFFFFFF;
global_const u64 max_u64     =  0xFFFFFFFFFFFFFFFF;

global_const i8  max_i8      =  127;
global_const i16 max_i16     =  32767;
global_const i32 max_i32     =  2147483647;
global_const i64 max_i64     =  9223372036854775807;

global_const i8  min_i8      = -127 - 1;
global_const i16 min_i16     = -32767 - 1;
global_const i32 min_i32     = -2147483647 - 1;
global_const i64 min_i64     = -9223372036854775807 - 1;

global_const f32 max_f32     =  3.402823466e+38f;
global_const f32 min_f32     = -3.402823466e+38f;

global_const f32 smallest_positive_f32 = 1.1754943508e-38f;
global_const f32 epsilon_f32 =  5.96046448e-8f;
global_const f32 pi_f32      =  3.14159265359f;
global_const f32 half_pi_f32 =  1.5707963267f;

global_const f64 max_f64     =  1.79769313486231e+308;
global_const f64 min_f64     = -1.79769313486231e+308;
global_const f64 smallest_positive_f64 = 4.94065645841247e-324;
global_const f64 epsilon_f64 =  1.11022302462515650e-16;

#define clamp_signed_to_i8(x)    (i8)  (clamp((i64)i8_min, (i64)(x), (i64)i8_max))
#define clamp_signed_to_i16(x)   (i16) (clamp((i64)i16_min, (i64)(x), (i64)i16_max))
#define clamp_signed_to_i32(x)   (i32) (clamp((i64)i32_min, (i64)(x), (i64)i32_max))
#define clamp_signed_to_i64(x)   (i64) (clamp((i64)i64_min, (i64)(x), (i64)i64_max))
#define clamp_unsigned_to_i8(x)  (i8)  (clamp_top((u64)(x), (u64)i8_max))
#define clamp_unsigned_to_i16(x) (i16) (clamp_top((u64)(x), (u64)i16_max))
#define clamp_unsigned_to_i32(x) (i32) (clamp_top((u64)(x), (u64)i32_max))
#define clamp_unsigned_to_i64(x) (i64) (clamp_top((u64)(x), (u64)i64_max))
#define clamp_signed_to_u8(x)    (u8)  (clamp_top((u64)clamp_bottom(0, (i64)(x)), (u64)u8_max))
#define clamp_signed_to_u16(x)   (u16) (clamp_top((u64)clamp_bottom(0, (i64)(x)), (u64)u16_max))
#define clamp_signed_to_u32(x)   (u32) (clamp_top((u64)clamp_bottom(0, (i64)(x)), (u64)u32_max))
#define clamp_signed_to_u64(x)   (u64) (clamp_top((u64)clamp_bottom(0, (i64)(x)), (u64)u64_max))
#define clamp_unsigned_to_u8(x)  (u8)  (clamp_top((u64)(x), (u64)u8_max))
#define clamp_unsigned_to_u16(x) (u16) (clamp_top((u64)(x), (u64)u16_max))
#define clamp_unsigned_to_u32(x) (u32) (clamp_top((u64)(x), (u64)u32_max))
#define clamp_unsigned_to_u64(x) (u64) (clamp_top((u64)(x), (u64)u64_max))

global_const u32 bit_1  = 0x00000001;
global_const u32 bit_2  = 0x00000002;
global_const u32 bit_3  = 0x00000004;
global_const u32 bit_4  = 0x00000008;
global_const u32 bit_5  = 0x00000010;
global_const u32 bit_6  = 0x00000020;
global_const u32 bit_7  = 0x00000040;
global_const u32 bit_8  = 0x00000080;
global_const u32 bit_9  = 0x00000100;
global_const u32 bit_10 = 0x00000200;
global_const u32 bit_11 = 0x00000400;
global_const u32 bit_12 = 0x00000800;
global_const u32 bit_13 = 0x00001000;
global_const u32 bit_14 = 0x00002000;
global_const u32 bit_15 = 0x00004000;
global_const u32 bit_16 = 0x00008000;
global_const u32 bit_17 = 0x00010000;
global_const u32 bit_18 = 0x00020000;
global_const u32 bit_19 = 0x00040000;
global_const u32 bit_20 = 0x00080000;
global_const u32 bit_21 = 0x00100000;
global_const u32 bit_22 = 0x00200000;
global_const u32 bit_23 = 0x00400000;
global_const u32 bit_24 = 0x00800000;
global_const u32 bit_25 = 0x01000000;
global_const u32 bit_26 = 0x02000000;
global_const u32 bit_27 = 0x04000000;
global_const u32 bit_28 = 0x08000000;
global_const u32 bit_29 = 0x10000000;
global_const u32 bit_30 = 0x20000000;
global_const u32 bit_31 = 0x40000000;
global_const u32 bit_32 = 0x80000000;

global_const u64 bit_33 = 0x0000000100000000;
global_const u64 bit_34 = 0x0000000200000000;
global_const u64 bit_35 = 0x0000000400000000;
global_const u64 bit_36 = 0x0000000800000000;
global_const u64 bit_37 = 0x0000001000000000;
global_const u64 bit_38 = 0x0000002000000000;
global_const u64 bit_39 = 0x0000004000000000;
global_const u64 bit_40 = 0x0000008000000000;
global_const u64 bit_41 = 0x0000010000000000;
global_const u64 bit_42 = 0x0000020000000000;
global_const u64 bit_43 = 0x0000040000000000;
global_const u64 bit_44 = 0x0000080000000000;
global_const u64 bit_45 = 0x0000100000000000;
global_const u64 bit_46 = 0x0000200000000000;
global_const u64 bit_47 = 0x0000400000000000;
global_const u64 bit_48 = 0x0000800000000000;
global_const u64 bit_49 = 0x0001000000000000;
global_const u64 bit_50 = 0x0002000000000000;
global_const u64 bit_51 = 0x0004000000000000;
global_const u64 bit_52 = 0x0008000000000000;
global_const u64 bit_53 = 0x0010000000000000;
global_const u64 bit_54 = 0x0020000000000000;
global_const u64 bit_55 = 0x0040000000000000;
global_const u64 bit_56 = 0x0080000000000000;
global_const u64 bit_57 = 0x0100000000000000;
global_const u64 bit_58 = 0x0200000000000000;
global_const u64 bit_59 = 0x0400000000000000;
global_const u64 bit_60 = 0x0800000000000000;
global_const u64 bit_61 = 0x1000000000000000;
global_const u64 bit_62 = 0x2000000000000000;
global_const u64 bit_63 = 0x4000000000000000;
global_const u64 bit_64 = 0x8000000000000000;

global_const u32 bitmask_1  = 0x00000001;
global_const u32 bitmask_2  = 0x00000003;
global_const u32 bitmask_3  = 0x00000007;
global_const u32 bitmask_4  = 0x0000000f;
global_const u32 bitmask_5  = 0x0000001f;
global_const u32 bitmask_6  = 0x0000003f;
global_const u32 bitmask_7  = 0x0000007f;
global_const u32 bitmask_8  = 0x000000ff;
global_const u32 bitmask_9  = 0x000001ff;
global_const u32 bitmask_10 = 0x000003ff;
global_const u32 bitmask_11 = 0x000007ff;
global_const u32 bitmask_12 = 0x00000fff;
global_const u32 bitmask_13 = 0x00001fff;
global_const u32 bitmask_14 = 0x00003fff;
global_const u32 bitmask_15 = 0x00007fff;
global_const u32 bitmask_16 = 0x0000ffff;
global_const u32 bitmask_17 = 0x0001ffff;
global_const u32 bitmask_18 = 0x0003ffff;
global_const u32 bitmask_19 = 0x0007ffff;
global_const u32 bitmask_20 = 0x000fffff;
global_const u32 bitmask_21 = 0x001fffff;
global_const u32 bitmask_22 = 0x003fffff;
global_const u32 bitmask_23 = 0x007fffff;
global_const u32 bitmask_24 = 0x00ffffff;
global_const u32 bitmask_25 = 0x01ffffff;
global_const u32 bitmask_26 = 0x03ffffff;
global_const u32 bitmask_27 = 0x07ffffff;
global_const u32 bitmask_28 = 0x0fffffff;
global_const u32 bitmask_29 = 0x1fffffff;
global_const u32 bitmask_30 = 0x3fffffff;
global_const u32 bitmask_31 = 0x7fffffff;

global_const u64 bitmask_32 = 0x00000000ffffffff;
global_const u64 bitmask_33 = 0x00000001ffffffff;
global_const u64 bitmask_34 = 0x00000003ffffffff;
global_const u64 bitmask_35 = 0x00000007ffffffff;
global_const u64 bitmask_36 = 0x0000000fffffffff;
global_const u64 bitmask_37 = 0x0000001fffffffff;
global_const u64 bitmask_38 = 0x0000003fffffffff;
global_const u64 bitmask_39 = 0x0000007fffffffff;
global_const u64 bitmask_40 = 0x000000ffffffffff;
global_const u64 bitmask_41 = 0x000001ffffffffff;
global_const u64 bitmask_42 = 0x000003ffffffffff;
global_const u64 bitmask_43 = 0x000007ffffffffff;
global_const u64 bitmask_44 = 0x00000fffffffffff;
global_const u64 bitmask_45 = 0x00001fffffffffff;
global_const u64 bitmask_46 = 0x00003fffffffffff;
global_const u64 bitmask_47 = 0x00007fffffffffff;
global_const u64 bitmask_48 = 0x0000ffffffffffff;
global_const u64 bitmask_49 = 0x0001ffffffffffff;
global_const u64 bitmask_50 = 0x0003ffffffffffff;
global_const u64 bitmask_51 = 0x0007ffffffffffff;
global_const u64 bitmask_52 = 0x000fffffffffffff;
global_const u64 bitmask_53 = 0x001fffffffffffff;
global_const u64 bitmask_54 = 0x003fffffffffffff;
global_const u64 bitmask_55 = 0x007fffffffffffff;
global_const u64 bitmask_56 = 0x00ffffffffffffff;
global_const u64 bitmask_57 = 0x01ffffffffffffff;
global_const u64 bitmask_58 = 0x03ffffffffffffff;
global_const u64 bitmask_59 = 0x07ffffffffffffff;
global_const u64 bitmask_60 = 0x0fffffffffffffff;
global_const u64 bitmask_61 = 0x1fffffffffffffff;
global_const u64 bitmask_62 = 0x3fffffffffffffff;
global_const u64 bitmask_63 = 0x7fffffffffffffff;

union V2_i8 {
  struct {
    i8 x;
    i8 y;
  };
  i8 v[2];
};
union V3_i8 {
  struct {
    i8 x;
    i8 y;
    i8 z;
  };
  struct {
    i8 r;
    i8 g;
    i8 b;
  };
  i8 v[3];
};
union V4_i8 {
  struct {
    i8 x;
    i8 y;
    i8 z;
    i8 w;
  };
  struct {
    i8 r;
    i8 g;
    i8 b;
    i8 a;
  };
  i8 v[4];
};
union V2_i16 {
  struct {
    i16 x;
    i16 y;
  };
  i16 v[2];
};
union V3_i16 {
  struct {
    i16 x;
    i16 y;
    i16 z;
  };
  struct {
    i16 r;
    i16 g;
    i16 b;
  };
  i16 v[3];
};
union V4_i16 {
  struct {
    i16 x;
    i16 y;
    i16 z;
    i16 w;
  };
  struct {
    i16 r;
    i16 g;
    i16 b;
    i16 a;
  };
  i16 v[4];
};
union V2_i32 {
  struct {
    i32 x;
    i32 y;
  };
  i32 v[2];
};
union V3_i32 {
  struct {
    i32 x;
    i32 y;
    i32 z;
  };
  struct {
    i32 r;
    i32 g;
    i32 b;
  };
  i32 v[3];
};
union V4_i32 {
  struct {
    i32 x;
    i32 y;
    i32 z;
    i32 w;
  };
  struct {
    i32 r;
    i32 g;
    i32 b;
    i32 a;
  };
  i32 v[4];
};
union V2_f32 {
  struct {
    f32 x;
    f32 y;
  };
  f32 v[2];
};
union V3_f32 {
  struct {
    f32 x;
    f32 y;
    f32 z;
  };
  struct {
    f32 r;
    f32 g;
    f32 b;
  };
  f32 v[3];
};
union V4_f32 {
  struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
  };
  struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
  };
  struct {
    f32 h;
    f32 s;
    f32 l;
    f32 __a;
  };
  f32 v[4];
};

union Range_i32 {
  struct {
    i32 min;
    i32 max;
  };
  struct {
    i32 start;
    i32 end;
  };
  struct {
    i32 first;
    i32 one_past_last;
  };
};
union Range_i64 {
  struct {
    i64 min;
    i64 max;
  };
  struct {
    i64 start;
    i64 end;
  };
  struct {
    i64 first;
    i64 one_past_last;
  };
};
union Range_u64 {
  struct {
    u64 min;
    u64 max;
  };
  struct {
    u64 start;
    u64 end;
  };
  struct {
    u64 first;
    u64 one_past_last;
  };
};
union Range_f32 {
  struct {
    f32 min;
    f32 max;
  };
  struct {
    f32 start;
    f32 end;
  };
  struct {
    f32 first;
    f32 one_past_last;
  };
};

struct Range_i32_Array {
  Range_i32 *ranges;
  i32 count;
};
struct Range_i64_Array {
  Range_i64 *ranges;
  i32 count;
};
struct Range_u64_Array {
  Range_u64 *ranges;
  i32 count;
};
struct Range_f32_Array {
  Range_f32 *ranges;
  i32 count;
};

union Rect_i16 {
  struct {
    i16 x0;
    i16 y0;
    i16 x1;
    i16 y1;
  };
  struct {
    V2_i16 p0;
    V2_i16 p1;
  };
  V2_i16 p[2];
};

union Rect_i32 {
  struct {
    i32 x0;
    i32 y0;
    i32 x1;
    i32 y1;
  };
  struct {
    V2_i32 p0;
    V2_i32 p1;
  };
  V2_i32 p[2];
};
union Rect_f32 {
  struct {
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
  };
  struct {
    V2_f32 p0;
    V2_f32 p1;
  };
  V2_f32 p[2];
};

union Rect_f32_Pair {
  struct {
    Rect_f32 a;
    Rect_f32 b;
  };
  struct {
    Rect_f32 min;
    Rect_f32 max;
  };
  struct {
    Rect_f32 pos;
    Rect_f32 uv;
  };
  struct {
    Rect_f32 e[2];
  };
};

typedef Rect_f32_Pair Quad;
typedef u32 ARGB_Color;

struct Array_i8 {
  i8 *vals;
  i32 count;
};
struct Array_i16 {
  i16 *vals;
  i32 count;
};
struct Array_i32 {
  i32 *vals;
  i32 count;
};
struct Array_i64 {
  i64 *vals;
  i32 count;
};

struct Array_u8 {
  u8 *vals;
  i32 count;
};
struct Array_u16 {
  u16 *vals;
  i32 count;
};
struct Array_u32 {
  u32 *vals;
  i32 count;
};
struct Array_u64 {
  u64 *vals;
  i32 count;
};

typedef u8  utf8;
typedef u16 utf16;
typedef u32 utf32;

struct String_Const_char {
  char *str;
  u64 size;
};
struct String_Const_utf8 {
  u8 *str;
  u64 size;
};
struct String_Const_utf16 {
  u16 *str;
  u64 size;
};
struct String_Const_utf32 {
  u32 *str;
  u64 size;
};

typedef i32 String_Encoding;
enum {
  StringEncoding_ASCII = 0,
  StringEncoding_UTF8  = 1,
  StringEncoding_UTF16 = 2,
  StringEncoding_UTF32 = 3,
};

struct String_Const_Any {
  String_Encoding encoding;
  union {
    struct {
      void *str;
      u64 size;
    };

    String_Const_char  s_char;
    String_Const_utf8  s_utf8;
    String_Const_utf16 s_utf16;
    String_Const_utf32 s_utf32;
  };
};

struct String_char {
  union {
    String_Const_char string;
    struct {
      char *str;
      u64 size;
    };
  };
  u64 cap;
};
struct String_utf8 {
  union {
    String_Const_utf8 string;
    struct {
      u8 *str;
      u64 size;
    };
  };
  u64 cap;
};
struct String_utf16 {
  union {
    String_Const_utf16 string;
    struct {
      u16 *str;
      u64 size;
    };
  };
  u64 cap;
};
struct String_utf32 {
  union {
    String_Const_utf32 string;
    struct {
      u32 *str;
      u64 size;
    };
  };
  u64 cap;
};

struct String_Any {
  String_Encoding encoding;
  union {
    struct {
      void *str;
      u64 size;
      u64 cap;
    };
    String_char  s_char;
    String_utf8  s_utf8;
    String_utf16 s_utf16;
    String_utf32 s_utf32;
  };
};

#define string_literal_init(s) {(s), sizeof(s) - sizeof(*s)}
#define string_literal_init_type(s, t) {(t *) (s), sizeof(s) - 1}
#define string_from_c_string(type, s, capacity) {(type *) s, concat(get_length_c_string_, type)((type *) s), (capacity)}
#define string_from_fixed_size(type, buffer) {(type *) (buffer), 0, sizeof(buffer) - sizeof(type)}
#define buffer_from_fixed_size(buffer) {(u8 *) (buffer), 0, sizeof(buffer)}

struct Buffer
{
  u8  *data;

  u64  used;
  u64  size;
};

struct File_Buffer
{
  u8  *data;

  String_Const_utf8 file_path;

  u64 size;
  u64 used;
};

// helpful functions
u32 count_set_bits(u64 bits)
{
    u32 count = 0;
    while (bits != 0)
    {
        bits = bits & (bits - 1);
        count++;
    }
    return count;
}

#define align(val, alignment) ((((val) + (alignment) - 1) / (alignment)) * (alignment))

// WELL512 rng, Chris Lomont, www.lomont.org

// initialize state to random bits
static u32 rng_state[16] = 
{
  0b10000101000010000101010101110010,
  0b10111110011111001111100010010110,
  0b11111100000010010011100000111001,
  0b11011000000001011100001000000101,
  0b10011100011101111001101101111101,
  0b11111010001101010001011101010011,
  0b10101001110010011111011111000001,
  0b01000011001011000000101100110111,
  0b01010111010101110110001110101010,
  0b11011011010111000110100010100100,
  0b11001010111101010000101010110000,
  0b00011001101000010011101011100000,
  0b01111010111010100110000001111111,
  0b01000100101010010010101100111100,
  0b00101000001010110011011011101101,
  0b00001111100000101110011010101011,
};

// init should also reset this to 0
static u32 rng_index = 0;

// return 32 bit random number
u32 WELLRNG512(void)
{
  u32 a, b, c, d;
  a = rng_state[rng_index];
  c = rng_state[(rng_index + 13) & 15];
  b = a ^ c ^ (a << 16) ^ (c << 15);
  c = rng_state[(rng_index + 9) & 15];
  c ^= (c >> 11);
  a = rng_state[rng_index] = b ^ c;
  d = a^((a << 5) & 0xDA442D24UL);
  rng_index = (rng_index + 15) & 15;
  a = rng_state[rng_index];
  rng_state[rng_index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
  return rng_state[rng_index];
}

void rng_init(void)
{
  rng_index = 0;
}

u32 rng_get_random32(void)
{
  u32 result = WELLRNG512();
  return(result);
}

u64 rng_fill_buffer(u8 *buffer, u64 buffer_length)
{
  u32 *buffer32 = (u32 *) buffer;
  u64 length32 = buffer_length / sizeof(*buffer32);

  for (u64 index32 = 0;
       index32 < length32;
       ++index32)
  {
    buffer32[index32] = rng_get_random32();
  }

  u64 remaining = buffer_length % sizeof(*buffer32);

  buffer += length32 * sizeof(*buffer32);
  for (u64 remaining_index = 0;
       remaining_index < remaining;
       ++remaining_index)
  {
    buffer[remaining_index] = (u8) rng_get_random32();
  }

  return(buffer_length);
}

#define TRADER_BASE_DEFINES_H
#endif
