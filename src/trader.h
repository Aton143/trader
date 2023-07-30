#ifndef TRADER_H

#define TRADER_VERSION 1LL
#include "trader_base_defines.h"

#if COMPILER_CL
# pragma warning(push)
# pragma warning(disable: 4996)
#elif COMPILER_GCC
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
# pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <stdarg.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#if COMPILER_CL
#  pragma warning(pop)
#elif COMPILER_GCC
#  pragma GCC diagnostic pop
#endif

struct Global_Platform_State;
struct Render_Context;

static f32 global_slider_float;

#if !SHIP_MODE
global u8 __debug_memory[1 << 20];
#endif

#if OS_WINDOWS
#  include <intrin.h>
# if ARCH_X64
#  include <immintrin.h>
# endif
#endif

#include "trader_math.cpp"
#include "trader_memory.cpp"
#include "trader_string_utilities.cpp"
#include "trader_utils.cpp"

#include "platform_linux/linux_implementation.cpp"

#include "trader_handle.cpp"
#include "trader_meta.cpp"

#include "trader_unicode.cpp"
#include "trader_text_edit.cpp"

#if COMPILER_CL
# include "compiler_impl/cl_impl.cpp"
#elif COMPILER_GCC
# include "compiler_impl/gcc_impl.cpp"
#elif COMPILER_CLANG
# include "compiler_impl/clang_impl.cpp"
#endif

global Text_Edit_Buffer debug_teb = make_text_edit_buffer({__debug_memory, array_count(__debug_memory), 0});

#include "trader_render.cpp"
#include "trader_ui.cpp"

#include "trader_network.cpp"
#include "trader_serialization.cpp"

#if OS_WINDOWS
# include "platform_win32/win32_implementation.h"
#endif

#define TRADER_H
#endif
