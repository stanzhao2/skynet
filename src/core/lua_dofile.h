

#ifndef __LUA_DOFILE_H
#define __LUA_DOFILE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

#define nameof_main "main"

SKYNET_API int lua_dofile(lua_State* L);
SKYNET_API typeof<io::service> skynet_service(int osid);

#define find_service(id) skynet_service(id)

/********************************************************************************/

#endif //__LUA_DOFILE_H
