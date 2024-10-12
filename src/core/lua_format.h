

#ifndef __LUA_FORMAT_H
#define __LUA_FORMAT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

/********************************************************************************/

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_format(lua_State* L);

/********************************************************************************/

#endif //__LUA_FORMAT_H
