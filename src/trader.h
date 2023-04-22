#ifndef TRADER_H

#define TRADER_VERSION 1LL

#include <openssl/base.h>

#include "trader_base_defines.h"

#include "trader_memory.h"
#include "trader_handle.h"

#include "trader_string_utilities.h"

#include "trader_platform.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "./foreign/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "./foreign/stb_truetype.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "./foreign/stb_sprintf.h"

#include "trader_font.h"
#include "trader_render.h"
#include "trader_network.h"

#if OS_WINDOWS
#include "platform_win32/win32_implementation.h"
#endif

#define TRADER_H
#endif
