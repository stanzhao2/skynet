

#ifndef __LUA_PCALL_H
#define __LUA_PCALL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_pcall(lua_State* L);
SKYNET_API int skynet_pcall (lua_State* L, int n, int r);
SKYNET_API int skynet_xpcall(lua_State* L, int n, int r);

/********************************************************************************/

#ifdef  lua_pcall
#undef  lua_pcall
#endif

#define lua_pcall(L, n, r)  skynet_pcall(L, n, r)
#define lua_xpcall(L, n, r) skynet_xpcall(L, n, r)

/********************************************************************************/

#endif //__LUA_PCALL_H
