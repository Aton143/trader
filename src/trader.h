#ifndef TRADER_H

#define TRADER_VERSION 1LL

#include "trader_base_defines.h"

#include "trader_memory.h"
#include "trader_string_utilities.h"

#include "trader_platform.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "./foreign/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "./foreign/stb_truetype.h"

#include "trader_font.h"

#if OS_WINDOWS
#include "platform_win32/win32_implementation.h"
#endif

#define TRADER_H
#endif
