

#include <set>
#include "lua_serialize.h"

/********************************************************************************/

#define push_marks(t, b, c) if (t == LUA_TSTRING) luaL_addchar(b, c)

static void dump_object(lua_State* L, int i, int lv, luaL_Buffer* b);

static void push_black(luaL_Buffer* b, int lv) {
  for (int i = 0; i < lv; i++) {
    luaL_addstring(b, "    ");
  }
}

static void dump_key(lua_State* L, int i, int lv, luaL_Buffer* b) {
  push_black(b, lv);
  lua_getglobal(L, "tostring");
  lua_pushvalue(L, i);
  if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
    int type = lua_type(L, i);
    luaL_addchar(b, '[');
    push_marks(type, b, '"');
    luaL_addstring(b, lua_tostring(L, -1));
    push_marks(type, b, '"');
    luaL_addchar(b, ']');
    luaL_addstring(b, " = ");
  }
  lua_pop(L, 1);
}

static void dump_table(lua_State* L, int i, int lv, luaL_Buffer* b) {
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

  luaL_addchar(b, '{');
  lua_pushfstring(L, " --table: %p", lua_topointer(L, i));
  luaL_addstring(b, lua_tostring(L, -1));
  luaL_addchar(b, ' ');
  lua_pop(L, 1);

  bool newline = true;
  lua_pushnil(L);
  while (lua_next(L, i)) {
    if (newline) {
      luaL_addchar(b, '\n');
      newline = false;
    }
    int top = lua_gettop(L);
    dump_key(L, top - 1, lv + 1, b);
    dump_object(L, top, lv + 1, b);
    luaL_addchar(b, ',');
    luaL_addchar(b, '\n');
    lua_pop(L, 1);
  }
  push_black(b, lv);
  luaL_addchar(b, '}');

#ifdef LUA_DEBUG
  readed.erase(lua_topointer(L, -1));
#endif
}

static void dump_value(lua_State* L, int i, int lv, luaL_Buffer* b) {
  lua_getglobal(L, "tostring");
  lua_pushvalue(L, i);
  if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
    int type = lua_type(L, i);
    push_marks(type, b, '"');
    luaL_addstring(b, lua_tostring(L, -1));
    push_marks(type, b, '"');
  }
  lua_pop(L, 1);
}

static void dump_object(lua_State* L, int i, int lv, luaL_Buffer* b) {
  lua_type(L, i) == LUA_TTABLE ? dump_table(L, i, lv, b) : dump_value(L, i, lv, b);
}

/********************************************************************************/

SKYNET_API void lua_serialize(lua_State* L, int index) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  dump_object(L, index, 0, &b);
  luaL_pushresult(&b);
}

/********************************************************************************/
