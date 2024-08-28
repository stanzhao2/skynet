

#include "lua_dofile.h"
#include "lua_require.h"

static std::mutex _mutex;
static std::map<int, typeof<io::service>> _services;

class service_hold final {
public:
  inline service_hold() {
    std::unique_lock<std::mutex> _lock(_mutex);
    auto ios = io::service::local();
    _services[ios->id()] = ios;
  }
  inline ~service_hold() {
    std::unique_lock<std::mutex> _lock(_mutex);
    auto ios = io::service::local();
    _services.erase(ios->id());
  }
};

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

SKYNET_API int lua_dofile(lua_State* L) {
  char name[2048];
  const char* filename = luaL_checkstring(L, 1);
  filename = normal(filename, name);

  lua_pushstring(L, filename);
  lua_replace(L, 1);
  lua_pushvalue(L, 1);
  lua_setglobal(L, "progname");

  service_hold hold;

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

SKYNET_API typeof<io::service> skynet_service(int servicdid) {
  std::unique_lock<std::mutex> _lock(_mutex);
  auto iter = _services.find(servicdid);
  return iter == _services.end() ? typeof<io::service>() : iter->second;
}

/********************************************************************************/
