

#include "lua_sheet.h"

static std::mutex _mutex;

static struct sheet_L {
  lua_State* L;
  inline ~sheet_L() { lua_close(L); }
  inline sheet_L() : L(luaL_newstate()) { luaL_openlibs(L); }
} sheet;

/********************************************************************************/

static int new_table(lua_State* L, const char* name);

static void load_table(lua_State* L, const char* name, int level) {
  const char* p = strchr(name, '|');
  if (!p) {
    p = strchr(name, '\0');
  }
  char stage[1024];
  memcpy(stage, name, p - name);
  stage[p - name] = '\0';
  level ? lua_getfield(L, -1, stage) : lua_getglobal(L, stage);
  if (lua_type(L, -1) == LUA_TNIL) {
    return;
  }
  name = (*p ? p + 1 : p);
  if (*name) {
    load_table(L, name, level + 1);
  }
}

static int length(lua_State* L) {
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = sheet.L;
  lua_auto_revert revert(GL);
  load_table(GL, luaL_checkstring(L, 1), 0);
  lua_pushinteger(L, luaL_len(GL, -1));
  return 1;
}

static int pairs(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  lua_pushstring(L, name);
  lua_pushcclosure(L,
    [](lua_State* L) {
      const char* name = luaL_checkstring(L, lua_upvalueindex(1));
      std::unique_lock<std::mutex> lock(_mutex);
      lua_State* GL = sheet.L;
      lua_auto_revert revert(GL);

      load_table(GL, name, 0);
      if (lua_type(L, 2) == LUA_TNIL) {
        lua_pushnil(GL);
      }
      else {
        lua_pushstring(GL, luaL_checkstring(L, 2));
      }
      int status = lua_next(GL, -2);
      return status ? (lua_xmove(GL, L, 2), 2) : 0;
    }, 1
  );
  return 1;
}

static int read_sheet(lua_State* L) {
  lua_pushfstring(L, "%s|%s", luaL_checkstring(L, 1), luaL_checkstring(L, 3));
  const char* name = luaL_checkstring(L, -1);
  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = sheet.L;
  lua_auto_revert revert(GL);

  load_table(GL, name, 0);
  if (lua_type(GL, -1) != LUA_TTABLE) {
    lua_xmove(GL, L, 1);
  }
  else {
    new_table(L, name);
  }
  return 1;
}

static int luac_export(lua_State* L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  const char* name = luaL_checkstring(L, 2);
  lua_rotate(L, 1, 1);
  lua_wrap(L, 1);
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);

  std::unique_lock<std::mutex> lock(_mutex);
  lua_State* GL = sheet.L;
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
    lua_State* GL = sheet.L;
    lua_auto_revert revert(GL);
    if (lua_getglobal(GL, name) != LUA_TTABLE) {
      lua_pushnil(L);
      return 1;
    }
  } while (false);
  return new_table(L, name);
}

static int new_table(lua_State* L, const char* name) {
  lua_newtable(L);
  lua_newtable(L);
  lua_pushstring(L, name);
  lua_pushcclosure(L, [](lua_State* L) {
    int i = lua_upvalueindex(1);
    lua_pushstring(L, luaL_checkstring(L, i));
    lua_insert(L, 1);
    return read_sheet(L);
  }, 1);
  lua_setfield(L, -2, "__index");

  lua_pushstring(L, name);
  lua_pushcclosure(L, [](lua_State* L) {
    int i = lua_upvalueindex(1);
    lua_pushstring(L, luaL_checkstring(L, i));
    lua_insert(L, 1);
    return length(L);
  }, 1);
  lua_setfield(L, -2, "__len");

  lua_pushstring(L, name);
  lua_pushcclosure(L, [](lua_State* L) {
    int i = lua_upvalueindex(1);
    lua_pushstring(L, luaL_checkstring(L, i));
    lua_insert(L, 1);
    return pairs(L);
  }, 1);
  lua_setfield(L, -2, "__pairs");

  lua_pushstring(L, name);
  lua_pushcclosure(L, [](lua_State* L) {
    int i = lua_upvalueindex(1);
    luaL_error(L, "%s is readonly", luaL_checkstring(L, i));
    return 0;
  }, 1);
  lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_sheet(lua_State* L) {
  const luaL_Reg methods[] = {
    { "quote",    luac_import },
    { "share",    luac_export },
    { NULL,       NULL        }
  };

  lua_getglobal(L, "table");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'table' from stack */
  return 0;
}

/********************************************************************************/
