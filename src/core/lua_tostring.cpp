

#include <set>
#include "lua_tostring.h"

/********************************************************************************/

#define append_if_string(t, s, c) if (t == LUA_TSTRING) s += c

static void dump_object(lua_State* L, int i, int lv, std::string& s);

static void append_black(std::string& s, int lv) {
  for (int i = 0; i < lv; i++) {
    s.append("    ", 4);
  }
}

static void dump_key(lua_State* L, int i, int lv, std::string& s) {
  size_t n = 0;
  append_black(s, lv);
  s += "[";
  int t = lua_type(L, i);
  append_if_string(t, s, "\"");
  s.append(luaL_tolstring(L, i, &n), n);
  append_if_string(t, s, "\"");
  s += "] = ";
  lua_pop(L, 1);
}

static void dump_value(lua_State* L, int i, int lv, std::string& s) {
  size_t n = 0;
  int t = lua_type(L, i);
  int x = lua_type(L, 1);
  append_if_string(t, s, "\"");
  s.append(luaL_tolstring(L, i, &n), n);
  append_if_string(t, s, "\"");
  lua_pop(L, 1);
}

static void dump_table(lua_State* L, int i, int lv, std::string& s) {
#ifdef LUA_DEBUG
  static thread_local std::set<const void*> readed;
  if (lv == 0) {
    readed.clear();
  }
  auto p = lua_topointer(L, -1);
  auto iter = readed.find(p);
  if (iter != readed.end()) {
    return;
  }
  readed.insert(p);
#endif

  char buf[128];
  const char* fmt = lv ? "{ -- %s" : "%s {";
  snprintf(buf, sizeof(buf), fmt, luaL_tolstring(L, i, nullptr));
  s += buf;
  lua_pop(L, 1);

  bool newline = true;
  lua_pushnil(L);
  while (lua_next(L, i)) {
    if (newline) {
      s += '\n';
      newline = false;
    }
    int top = lua_gettop(L);
    dump_key(L, top - 1, lv + 1, s);
    dump_object(L, top, lv + 1, s);
    s += ",\n";
    lua_pop(L, 1);
  }
  append_black(s, lv);
  s += "}";

#ifdef LUA_DEBUG
  readed.erase(lua_topointer(L, -1));
#endif
}

static void dump_nil(lua_State* L, int i, std::string& s) {
  s += "nil";
}

static void dump_other(lua_State* L, int i, std::string& s) {
  s += luaL_tolstring(L, i, NULL);
  lua_pop(L, 1);
}

static void dump_numbert(lua_State* L, int i, std::string& s) {
  s += lua_isinteger(L, i) ? "integer: " : "number: ";
  s += luaL_tolstring(L, i, NULL);
  lua_pop(L, 1);
}

static void dump_string(lua_State* L, int i, std::string& s) {
  s += "string: ";
  s += luaL_checkstring(L, i);
}

static void dump_boolean(lua_State* L, int i, std::string& s) {
  s += "boolean: ";
  s += lua_toboolean(L, i) ? "true" : "false";
}

static void dump_object(lua_State* L, int i, int lv, std::string& s) {
  int type = lua_type(L, i);
  if (type == LUA_TTABLE) {
    dump_table(L, i, lv, s);
    return;
  }
  if (lua_type(L, 1) == LUA_TTABLE) {
    dump_value(L, i, lv, s);
    return;
  }
  switch (type) {
  case LUA_TNIL:     dump_nil(L, i, s);     return;
  case LUA_TBOOLEAN: dump_boolean(L, i, s); return;
  case LUA_TNUMBER:  dump_numbert(L, i, s); return;
  case LUA_TSTRING:  dump_string(L, i, s);  return;
  }
  dump_other(L, i, s);
}

static int luac_tostring(lua_State* L) {
  luaL_checkany(L, 1);
  if (lua_isnone(L, 2)) {
    luaL_tolstring(L, 1, NULL);
    return 1;
  }
  if (luaL_optboolean(L, 2, 0) == 0) {
    luaL_tolstring(L, 1, NULL);
    return 1;
  }
  std::string s;
  dump_object(L, 1, 0, s);
  lua_pushlstring(L, s.c_str(), s.size());
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_tostring(lua_State* L) {
  const luaL_Reg methods[] = {
    { "tostring",   luac_tostring },
    { NULL,         NULL          }
  };
  return new_module(L, LUA_GNAME, methods);
}

/********************************************************************************/
