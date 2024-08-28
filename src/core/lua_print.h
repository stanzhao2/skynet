

#ifndef __LUA_PRINT_H
#define __LUA_PRINT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

/********************************************************************************/

#include "../skynet_lua.h"

/********************************************************************************/

enum struct color_type {
  print, trace, error
};

SKYNET_API void lua_printf(
  color_type color, const char* fmt, ...
);

#define lua_fprint(f, ...) lua_printf(color_type::print, f, __VA_ARGS__)
#define lua_ftrace(f, ...) lua_printf(color_type::trace, f, __VA_ARGS__)
#define lua_ferror(f, ...) lua_printf(color_type::error, f, __VA_ARGS__)

/********************************************************************************/

SKYNET_API int luaopen_print(lua_State* L);

/********************************************************************************/

#endif //__LUA_PRINT_H
