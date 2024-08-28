

#include "lua_dofile.h"
#include "lua_require.h"

/********************************************************************************/

static const char* normal(const char* filename, char* out) {
  strcpy(out, filename);
  for (char *p = out; *p; p++) {
    char c = *p;
    char next_c = *(p + 1);
    if (c == '/' || c == '\\') {
      *p = '.';
      continue;
    }
    if (c == '.') {
      if (stricmp(p, ".lua") == 0) {
        *p = 0;
        break;
      }
    }
  }
  return out;
}

SKYNET_API int skynet_dofile(lua_State* L) {
  char name[2048];
  const char* filename = luaL_checkstring(L, 1);
  filename = normal(filename, name);

  lua_pushstring(L, filename);
  lua_replace(L, 1);
  lua_pushvalue(L, 1);
  lua_setglobal(L, "progname");

  int top = lua_gettop(L);
  lua_getglobal(L, LUA_LOADLIBNAME);  /* package */
  lua_getfield(L, -1, LUA_SEARCHERS);  /* searchers or loaders */
  lua_rawgeti(L, -1, 2);
  lua_pushvalue(L, 1);
  int result = lua_pcall(L, 1, LUA_MULTRET);
  if (result != LUA_OK) {
    lua_error(L);
  }
  int retcnt = lua_gettop(L) - top - 2;
  if (retcnt != 2) {
    lua_error(L);
  }
  result = lua_pcall(L, 1, 0);
  if (result != LUA_OK) {
    lua_error(L);
  }
  lua_settop(L, top);
  lua_getglobal(L, nameof_main);
  if (lua_isfunction(L, -1)) {
    lua_rotate(L, 2, 1);
    if (lua_pcall(L, top - 1, 0) != LUA_OK) {
      lua_error(L);
    }
  }
  return LUA_OK;
}

/********************************************************************************/
