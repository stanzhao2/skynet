

#ifndef __LUA_HTTP_PARSER_H
#define __LUA_HTTP_PARSER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_http(lua_State* L);

/********************************************************************************/

#endif //__LUA_HTTP_PARSER_H
