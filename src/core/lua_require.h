

#ifndef __LUA_REQUIRE_H
#define __LUA_REQUIRE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM < 502
# define LUA_SEARCHERS "loaders"
# define LUA_LOADFUNC(a, b, c, d) lua_load(a, b, c, d)
#else
# define LUA_SEARCHERS "searchers"
# define LUA_LOADFUNC(a, b, c, d) lua_load(a, b, c, d, 0)
#endif

/********************************************************************************/

SKYNET_API int luaopen_require(lua_State* L);

/********************************************************************************/

#endif //__LUA_REQUIRE_H
