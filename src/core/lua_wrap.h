

#ifndef __LUA_WRAP_H
#define __LUA_WRAP_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_wrap (lua_State* L);
SKYNET_API int skynet_wrap  (lua_State* L, int n);
SKYNET_API int skynet_unwrap(lua_State* L);

#define lua_wrap(L, n) skynet_wrap(L, n)
#define lua_unwrap(L)  skynet_unwrap(L)

/********************************************************************************/

#endif //__LUA_WRAP_H
