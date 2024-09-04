

#include "lua_global.h"

/********************************************************************************/

struct global final {
  static int check_access(lua_State* L) {
    lua_Debug ar;
    if (lua_getstack(L, 1, &ar) == 0) {
      lua_rawset(L, 1);
      return 0;
    }
    if (lua_getinfo(L, "Sln", &ar) && ar.currentline < 0) {
      lua_rawset(L, 1);
      return 0;
    }
    const char* name = luaL_checkstring(L, -2);
    if (lua_isfunction(L, -1)) {
      if (strcmp(name, nameof_main) == 0) {
        lua_rawset(L, 1);
        return 0;
      }
    }
    return luaL_error(L, "Cannot use global variables: %s", name);
  }
  static int open_library(lua_State* L) {
    lua_getglobal(L, LUA_GNAME);
    lua_getfield(L, -1, "__newindex");
    if (lua_isfunction(L, -1)) {
      lua_pop(L, 2);
      return 0;
    }
    lua_pop(L, 1);
    luaL_newmetatable(L, "placeholders");
    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, check_access);

    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
    return 0;
  }
};

/********************************************************************************/

SKYNET_API int luaopen_global(lua_State* L) {
  return global::open_library(L);
}

/********************************************************************************/
