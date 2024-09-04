

#ifndef __LUA_CORE_H
#define __LUA_CORE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <tinydir.h>
#include <luaset.h>
#include <conv.h>
#include <stdnet.h>
using namespace stdnet;

/********************************************************************************/

#define lua_success(s) (s == LUA_OK || s == LUA_YIELD)
#define lua_yield_k(L, n, ctx, f)  lua_yieldk(L, n, (lua_KContext)ctx, f)
#define lua_pcall_k(L, n, r, k, f) lua_pcallk(L, n, r, 0, (lua_KContext)k, f)

/********************************************************************************/

SKYNET_API int luaopen_core(lua_State *L);

/********************************************************************************/

#endif //__LUA_CORE_H
