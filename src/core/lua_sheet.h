

#ifndef __LUA_SHEET_H
#define __LUA_SHEET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

/********************************************************************************/

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_sheet(lua_State* L);

/********************************************************************************/

#endif //__LUA_SHEET_H
