

#include "lua_path.h"

/********************************************************************************/

static const char* luapath[] = {
  "lua" LUA_DIRSEP "?.lua", "lua" LUA_DIRSEP "?" LUA_DIRSEP "init.lua", NULL
};

static const char* luacpath[] = {
  "lib" LUA_DIRSEP "lua" LUA_DIRSEP "?" LIBEXT, NULL
};

/********************************************************************************/

static void init_path(lua_State* L, const char* name, const char* path[]) {
  luaL_Buffer buf;
  luaL_buffinit(L, &buf);
  for (int i = 0; path[i]; i++) {
    luaL_addstring(&buf, path[i]);
    luaL_addchar(&buf, ';');
  }
  char dir[MAX_PATH];
  if (getpwd(dir, sizeof(dir))) {
    for (int i = 0; path[i]; i++) {
      luaL_addstring(&buf, dir);
      luaL_addstring(&buf, LUA_DIRSEP);
      luaL_addstring(&buf, path[i]);
      if (path[i + 1]) {
        luaL_addchar(&buf, ';');
      }
    }
  }
  luaL_pushresult(&buf);
  lua_getglobal(L, LUA_LOADLIBNAME);
  lua_rotate   (L, -2, -1);
  lua_setfield (L, -2, name);
  lua_pop(L, 1);
}

/********************************************************************************/

SKYNET_API int luaopen_path(lua_State* L) {
  init_path(L, "path",  luapath);
  init_path(L, "cpath", luacpath);
  return 0;
}

/********************************************************************************/
