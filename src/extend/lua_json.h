

#ifndef __LUA_RAPIDJSON_H
#define __LUA_RAPIDJSON_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_json(lua_State* L);

/********************************************************************************/

#endif //__LUA_RAPIDJSON_H
