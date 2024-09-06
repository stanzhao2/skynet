

#include "lua_sheet.h"

static std::mutex _mutex;

static struct globalL {
  lua_State* L;
  inline ~globalL() { lua_close(L); }
  inline globalL() : L(luaL_newstate()) { luaL_openlibs(L); }
} global;

#define is_continue(type, name) (type == LUA_TTABLE && *name)

/********************************************************************************/

static int new_table(lua_State* L, const char* name);

static int push_table(lua_State* L, const char* name, int level) {
  char stage[1024];
  const char* p = strchr(name, '|');
  p = p ? p : strchr(name, '\0');
  memcpy(stage, name, p - name);
  stage[p - name] = '\0';
  level ? lua_getfield(L, -1, stage) : lua_getglobal(L, stage);
  name = (*p ? p + 1 : p);
  auto type = lua_type(L, -1);
  return is_continue(type, name) ? push_table(L, name, level + 1) : type;
}

static int length(lua_State* L) {
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = global.L;
  lua_auto_revert revert(GL);
  push_table(GL, luaL_checkstring(L, 1), 0);
  lua_pushinteger(L, luaL_len(GL, -1));
  return 1;
}

static int iterator(lua_State* L) {
  const char* name = luaL_checkstring(L, lua_upvalueindex(1));
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = global.L;
  lua_auto_revert revert(GL);
  push_table(GL, name, 0);
  lua_xmove(L, GL, 1);
  return lua_next(GL, -2) ? (lua_xmove(GL, L, 2), 2) : 0;
}

static int pairs(lua_State* L) {
  lua_pushstring(L, luaL_checkstring(L, 1));
  lua_pushcclosure(L, iterator, 1);
  return 1;
}

static int newindex(lua_State* L) {
  luaL_error(L, "%s is readonly", luaL_checkstring(L, 1));
  return 0;
}

static int check_table(lua_State* L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  if (lua_getmetatable(L, 1)) {
    lua_pushstring(L, "export");
    lua_rawget(L, -2);
    if (lua_type(L, -1) == LUA_TBOOLEAN) {
      if (lua_toboolean(L, -1)) return LUA_ERRRUN;
    }
  }
  return LUA_OK;
}

static int read_sheet(lua_State* L) {
  lua_pushfstring(L, "%s|%s", luaL_checkstring(L, 1), luaL_checkstring(L, 3));
  const char* name = luaL_checkstring(L, -1);
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = global.L;
  lua_auto_revert revert(GL);
  int type = push_table(GL, name, 0);
  return (type == LUA_TTABLE) ? new_table(L, name) : (lua_xmove(GL, L, 1), 1);
}

static int luac_delete(lua_State* L) {
  const char* name = luaL_checkstring(L, 2);
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = global.L;
  lua_auto_revert revert(GL);
  lua_pushnil(GL);
  lua_setglobal(GL, name);
  return 0;
}

static int luac_export(lua_State* L) {
  if (lua_type(L, 1) == LUA_TNIL) {
    return luac_delete(L);
  }
  if (check_table(L) != LUA_OK) {
    luaL_error(L, "export repeat");
  }
  const char* name = luaL_checkstring(L, 2);
  lua_rotate(L, 1, 1);
  lua_wrap(L, 1);
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);

  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = global.L;
  lua_auto_revert revert(GL);
  lua_pushlstring(GL, data, size);
  lua_unwrap(GL);
  lua_setglobal(GL, name);
  return 0;
}

static int luac_import(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  do {
    std::unique_lock<std::mutex> lock(_mutex);
    lua_State* GL = global.L;
    lua_auto_revert revert(GL);
    if (lua_getglobal(GL, name) != LUA_TTABLE) {
      lua_pushnil(L);
      return 1;
    }
  } while (false);
  return new_table(L, name);
}

static int new_proxy(lua_State* L, lua_CFunction f) {
  lua_pushstring(L, luaL_checkstring(L, lua_upvalueindex(1)));
  lua_insert(L, 1);
  return f(L);
}

#define set_metamethod(L, name, what, f) { \
  lua_pushstring(L, name); \
  lua_pushcclosure(L, [](lua_State* L) { return new_proxy(L, f); }, 1); \
  lua_setfield(L, -2, what); \
}

static int new_table(lua_State* L, const char* name) {
  lua_newtable(L);
  lua_newtable(L);
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "export");  
  set_metamethod(L, name, "__index",    read_sheet);
  set_metamethod(L, name, "__len",      length);
  set_metamethod(L, name, "__pairs",    pairs);
  set_metamethod(L, name, "__newindex", newindex);
  lua_setmetatable(L, -2);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_sheet(lua_State* L) {
  const luaL_Reg methods[] = {
    { "export",   luac_export },
    { "import",   luac_import },
    { NULL,         NULL      }
  };

  lua_getglobal(L, "table");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'table' from stack */
  return 0;
}

/********************************************************************************/
