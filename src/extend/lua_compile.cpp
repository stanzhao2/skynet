

#include "lua_compile.h"

/********************************************************************************/

static int dump_writer(lua_State* L, const void* p, size_t sz, void* ud) {
  return fwrite(p, 1, sz, (FILE*)ud), LUA_OK;
}

static int dump_string(lua_State* L, const void* p, size_t sz, void* ud) {
  std::string* ps = (std::string*)ud;
  return ps->append((char*)p, sz), LUA_OK;
}

static int luac_compile(lua_State* L) {
  const char* infile  = luaL_checkstring(L, 1);
  const char* outfile = luaL_optstring(L, 2, nullptr);
  if (luaL_loadfile(L, infile) != LUA_OK) {
    lua_pushboolean(L, 0);
    return 1;
  }
  int result;
  if (outfile == nullptr) {
    std::string data;
    result = lua_dump(L, dump_string, &data, 0);
    if (result != LUA_OK) {
      lua_pushnil(L);
    }
    else {
      lua_pushlstring(L, data.c_str(), data.size());
    }
    return 1;
  }
  FILE* fpw = fopen(outfile, "wb");
  if (fpw == nullptr) {
    lua_pushboolean(L, 0);
    return 1;
  }
  result = lua_dump(L, dump_writer, fpw, 0);
  lua_pushboolean(L, (result == LUA_OK) ? 1 : 0);
  fclose(fpw);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_compile(lua_State* L) {
  const luaL_Reg methods[] = {
    { "compile",  luac_compile  }, /* os.compile(inname, outname) */
    { NULL,       NULL          }
  };
  return new_module(L, "os", methods);
}

/********************************************************************************/
