

#include "lua_pcall.h"

/********************************************************************************/

static lua_State *getthread(lua_State *L, int *arg) {
  if (lua_isthread(L, 1)) {
    *arg = 1;
    return lua_tothread(L, 1);
  }
  *arg = 0;
  return L;  /* function will operate over current thread */
}

static int traceback(lua_State *L) {
  int arg;
  lua_State *L1 = getthread(L, &arg);
  const char *msg = lua_tostring(L, arg + 1);
  if (strstr(msg, "stack traceback")) {
    return 1;
  }
  if (msg == NULL && !lua_isnoneornil(L, arg + 1))  /* non-string 'msg'? */
    lua_pushvalue(L, arg + 1);  /* return it untouched */
  else {
    int level = (int)luaL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
    luaL_traceback(L, L1, msg, level);
  }
  return 1;
}

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_pushboolean(L, 0);  /* first result (false) */
    lua_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  return lua_gettop(L) - (int)extra;  /* return all results */
}

static int luac_xpcall(lua_State *L) {
  int status;
  int n = lua_gettop(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
  lua_pushboolean(L, 1);  /* first result */
  lua_pushvalue(L, 1);  /* function */
  lua_rotate(L, 3, 2);  /* move them below function's arguments */
  status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}

static int luac_pcall(lua_State* L) {
  lua_pushcfunction(L, traceback);
  lua_insert(L, 2);
  return luac_xpcall(L);
}

/********************************************************************************/

SKYNET_API int lua_xpcall(lua_State* L, int n, int r) {
  int top = lua_gettop(L);
  lua_pushcfunction(L, traceback);
  int errfunc = top - n;
  lua_insert(L, errfunc);
  int status = lua_pcallk(L, n, r, errfunc, 0, 0);
  top = lua_gettop(L);
  lua_remove(L, errfunc); /* remove traceback from stack */
  return status;
}

SKYNET_API int luaopen_pcall(lua_State* L) {
  static const luaL_Reg methods[] = {
    { "pcall",    luac_pcall    }, /* pcall (f [, arg1, ...]) */
    { "xpcall",   luac_xpcall   }, /* xpcall(f, msgh [, arg1, ...]) */
    { NULL,       NULL          }
  };
  return new_module(L, LUA_GNAME, methods);
}

/********************************************************************************/
