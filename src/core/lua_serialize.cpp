

#include <set>
#include "lua_serialize.h"

/********************************************************************************/

#define push_marks(t, s, c) if (t == LUA_TSTRING) s += c

static void dump_object(lua_State* L, int i, int lv, std::string& s);

static void push_black(std::string& s, int lv) {
  for (int i = 0; i < lv; i++) {
    s.append("    ", 4);
  }
}

static void dump_key(lua_State* L, int i, int lv, std::string& s) {
  int type = lua_type(L, i);
  push_black(s, lv);
  s += "[";
  push_marks(type, s, "\"");
  size_t n = 0;
  const char* p = luaL_tolstring(L, i, &n);
  s.append(p, n);
  push_marks(type, s, "\"");
  s += "] = ";
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

  s += "{";
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
    top = lua_gettop(L);
    lua_pop(L, 1);
  }
  push_black(s, lv);
  s += "}";

#ifdef LUA_DEBUG
  readed.erase(lua_topointer(L, -1));
#endif
}

static void dump_value(lua_State* L, int i, int lv, std::string& s) {
  int type = lua_type(L, i);
  push_marks(type, s, "\"");
  size_t n = 0;
  const char* p = luaL_tolstring(L, i, &n);
  s.append(p, n);
  push_marks(type, s, "\"");
  lua_pop(L, 1);
}

static void dump_object(lua_State* L, int i, int lv, std::string& s) {
  lua_type(L, i) == LUA_TTABLE ? dump_table(L, i, lv, s) : dump_value(L, i, lv, s);
}

/********************************************************************************/

SKYNET_API void lua_serialize(lua_State* L, int index) {
  std::string s;
  dump_object(L, index, 0, s);
  lua_pushlstring(L, s.c_str(), s.size());
}

/********************************************************************************/
