

#ifndef __LUA_SKIPLIST_H
#define __LUA_SKIPLIST_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_skiplist(lua_State* L);

/********************************************************************************/

#endif //__LUA_SKIPLIST_H
