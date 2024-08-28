

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

SKYNET_API int luaopen_core(lua_State *L);

/********************************************************************************/

#endif //__LUA_CORE_H
