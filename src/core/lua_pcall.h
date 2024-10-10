

#ifndef __LUA_PCALL_H
#define __LUA_PCALL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_pcall(lua_State* L);
SKYNET_API int lua_xpcall(lua_State* L, int n, int r, int f);

/********************************************************************************/

#ifdef  lua_pcall
#undef  lua_pcall
#define lua_pcall(L, n, r, f) lua_xpcall(L, n, r, f)
#endif

/********************************************************************************/

#endif //__LUA_PCALL_H
