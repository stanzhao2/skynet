

#ifndef __LUA_PCALL_H
#define __LUA_PCALL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_pcall(lua_State* L);

/********************************************************************************/

#endif //__LUA_PCALL_H
