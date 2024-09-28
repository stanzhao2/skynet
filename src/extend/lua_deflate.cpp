

#include "lua_deflate.h"

#ifdef STDNET_USE_DEFLATE

/********************************************************************************/

static int gzip_deflate(lua_State *L) {
  size_t size = 0;
  const char* data = (char*)luaL_checklstring(L, 1, &size);
  std::string type = luaL_optstring(L, 2, "deflate");
  if (type != "gzip" && type != "deflate") {
    luaL_error(L, "unknown type: %s", type);
  }
  std::string output;
  bool gzip = (type == "gzip") ? true : false;
  if (!do_deflate(data, size, gzip, output)) {
    lua_pushnil(L);
  }
  else {
    lua_pushlstring(L, output.c_str(), output.size());
  }
  return 1;
}

static int gzip_inflate(lua_State *L) {
  size_t size = 0;
  const char* data = (char*)luaL_checklstring(L, 1, &size);
  std::string type = luaL_optstring(L, 2, "inflate");
  if (type != "gzip" && type != "inflate") {
    luaL_error(L, "unknown type: %s", type);
  }
  std::string output;
  bool gzip = (type == "gzip") ? true : false;
  if (!do_inflate(data, size, gzip, output)){
    lua_pushnil(L);
  }
  else {
    lua_pushlstring(L, output.c_str(), output.size());
  }
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_deflate(lua_State* L) {
  const luaL_Reg methods[] = {
    { "compress",   gzip_deflate   },  /* compress   */
    { "uncompress", gzip_inflate   },  /* uncompress */
    { NULL,         NULL           },
  };
  return new_module(L, LUA_GNAME, methods);
}

/********************************************************************************/

#else

SKYNET_API int luaopen_deflate(lua_State* L) { return 0; }

#endif //STDNET_USE_DEFLATE
