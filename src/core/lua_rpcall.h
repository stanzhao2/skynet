

#ifndef __LUA_RPCALL_H
#define __LUA_RPCALL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

#define evr_deliver   "deliver"
#define evr_bind      "bind"
#define evr_unbind    "unbind"
#define evr_response  "response"

/********************************************************************************/

SKYNET_API int luaopen_rpcall(lua_State* L);
SKYNET_API int lua_l_lookout (lua_CFunction f);
SKYNET_API int lua_l_rpcall  (lua_State* L);
SKYNET_API int lua_r_bind    (const char* name, size_t who, int rcb, int opt);
SKYNET_API int lua_r_unbind  (const char* name, size_t who, int* opt);
SKYNET_API int lua_r_deliver (const char* name, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf, size_t sn);
SKYNET_API int lua_r_response(const std::string& data, size_t caller, int rcf, size_t sn);

/********************************************************************************/

#endif //__LUA_RPCALL_H
