

#include "lua_bind.h"
#include "lua_pcall.h"

/********************************************************************************/

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_error(L);
  }
  return lua_gettop(L) - (int)extra;  /* return all results */
}

static int callback(lua_State* L) {
  int index = lua_upvalueindex(1); /* get the first up-value */
  int up_values = (int)luaL_checkinteger(L, index); /* get the number of up-values */
  lua_checkstack(L, up_values);
  for (int i = 2; i <= up_values; i++) {
    lua_pushvalue(L, lua_upvalueindex(i));
  }
  if (up_values > 1) { /* if the function has bound parameters */
    lua_rotate(L, 1, up_values - 1); /* rotate the calling parameters after the binding parameters */
  }
  int n = lua_gettop(L) - 1; /* calculate the number of all parameters */
  int status = lua_pcall_k(L, n, LUA_MULTRET, 0, finishpcall);
  return finishpcall(L, status, 0);
}

static int luac_bind(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION); /* function type check */
  int up_values = lua_gettop(L) + 1; /* number of up-values (function + argvs)*/
  lua_pushinteger(L, up_values); /* push up-values onto the stack */
  lua_rotate(L, 1, 1); /* rotate the function to the top of the stack */
  lua_pushcclosure(L, callback, up_values);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_bind(lua_State* L) {
  const luaL_Reg methods[] = {
    { "bind",   luac_bind   }, /* bind(f [, arg1, ...]) */
    { NULL,     NULL        }
  };
  return new_module(L, LUA_GNAME, methods);
}

/********************************************************************************/
