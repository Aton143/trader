#ifndef TRADER_H

#include <stdarg.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

struct Global_Platform_State;
struct Render_Context;

#define TRADER_VERSION 1LL
#include "trader_base_defines.h"

#include "trader_memory.h"
#include "trader_handle.h"
#include "trader_string_utilities.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "./foreign/stb_sprintf.h"

#include "trader_render.h"
#include "trader_font.h"
#include "trader_platform.h"
#include "trader_network.h"

#if OS_WINDOWS
#include "platform_win32/win32_implementation.h"
#endif

#include "trader_handle_impl.h"

#define TRADER_H
#endif
