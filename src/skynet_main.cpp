

#include "skynet_main.h"
#include "core/lua_print.h"
#include "core/lua_dofile.h"

/***********************************************************************************/

static bool is_integer(const char* p) {
  while (*p) {
    if (*p < '0' || *p > '9') return false;
    p++;
  }
  return true;
}

static bool is_number(const char* p) {
  int dotcnt = 0;
  while (*p) {
    if (*p < '0' || *p > '9') {
      if (*p != '.' || dotcnt++) {
        return false;
      }
    }
    p++;
  }
  return true;
}

static bool is_boolean(const char* p) {
  return (strcmp(p, "true") == 0 || strcmp(p, "false") == 0);
}

static void pushargv(lua_State* L, int argc, const char** argv) {
  for (int i = 0; i < argc; i++) {
    const char* p = argv[i];
    if (is_integer(p)) {
      lua_pushinteger(L, std::stoll(p));
      continue;
    }
    if (is_number(p)) {
      lua_pushnumber(L, std::stod(p));
      continue;
    }
    if (is_boolean(p)) {
      lua_pushboolean(L, strcmp(p, "true") == 0 ? 1 : 0);
      continue;
    }
    lua_pushstring(L, p);
  }
}

static int pmain(lua_State* L) {
  int argc = (int)luaL_checkinteger(L, 1);
  const char** argv = (const char**)lua_touserdata(L, 2);
  lua_pushcfunction(L, lua_dofile);
  pushargv(L, argc, argv);
  if (lua_pcall(L, argc, 0, 0) != LUA_OK) {
    lua_error(L);
  }
  return 0;
}

/***********************************************************************************/

SKYNET_API void skynet_main(lua_State* L, int argc, const char* argv[]) {
  error_code ec;
  auto local = lua_service();
  local->signal().add(SIGINT,  ec);
  local->signal().add(SIGTERM, ec);
  local->signal().async_wait(
    [local](const error_code& ec, int code) {
      local->stop();
      local->cancel();
    }
  );
  lua_pushcfunction(L, pmain);
  lua_pushinteger(L, argc);
  lua_pushlightuserdata(L, argv);
  if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
    lua_ferror("%s\n", luaL_checkstring(L, -1));
  }
  local->signal().clear();
}

/***********************************************************************************/
