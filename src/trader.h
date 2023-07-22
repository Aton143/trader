#ifndef TRADER_H

#pragma warning(push)
#pragma warning(disable: 4996)

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define TRADER_VERSION 1LL
#include "trader_base_defines.h"

#include <stdarg.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma warning(pop)

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

#include "trader_math.h"
#include "trader_meta.h"

#include "trader_memory.h"
#include "trader_unicode.h"
#include "trader_handle.h"
#include "trader_string_utilities.h"
#include "trader_text_edit.h"

global Text_Edit_Buffer debug_teb = make_text_edit_buffer({__debug_memory, array_count(__debug_memory), 0});

#include "trader_render.h"
#include "trader_font.h"
#include "trader_ui.h"
#include "trader_platform.h"
#include "trader_network.h"
#include "trader_serialization.h"

#if OS_WINDOWS
# include "platform_win32/win32_implementation.h"
#endif

#if COMPILER_CL
# include "compiler_impl/cl_impl.h"
#elif COMPILER_GCC
# include "compiler_impl/gcc_impl.h"
#elif COMPILER_CLANG
# include "compiler_impl/clang_impl.h"
#endif

#include "trader_meta_impl.h"
#include "trader_handle_impl.h"
#include "trader_ui_impl.h"

#define TRADER_H
#endif
