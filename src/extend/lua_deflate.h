

#ifndef __LUA_DEFALTE_H
#define __LUA_DEFALTE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/*
** gzip.deflate(data[, is_gzip]);
** gzip.inflate(data[, is_gzip]);
*/

/********************************************************************************/

SKYNET_API int luaopen_deflate(lua_State* L);

/********************************************************************************/

#endif //__LUA_DEFALTE_H
