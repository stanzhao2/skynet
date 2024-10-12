

#ifndef __LUA_SERIALIZE_H
#define __LUA_SERIALIZE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

/********************************************************************************/

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API void lua_serialize(lua_State* L, int index);

/********************************************************************************/

#endif //__LUA_SERIALIZE_H
