

#ifndef LUASET_H
#define LUASET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <lua.hpp>

#ifndef SKYNET_API
#define SKYNET_API
#endif

/***********************************************************************************/

inline void luac_unref(lua_State* L, int i) {
  luaL_unref(L, LUA_REGISTRYINDEX, i);
}

inline int luac_ref(lua_State* L, int i) {
  lua_pushvalue(L, i);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

inline int luac_rawgeti(lua_State* L, int i) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, i);
  return lua_type(L, -1);
}

#define lua_ref(L, i)      luac_ref(L, i)
#define lua_unref(L, i)    luac_unref(L, i)
#define lua_pushref(L, i)  luac_rawgeti(L, i)

/***********************************************************************************/

class unref_if_return final {
  unref_if_return(const unref_if_return&) = delete;
  int _unref;
  int _ref;
  lua_State* _L;

public:
  inline unref_if_return(lua_State* L, int r)
    : _L(L), _ref(r), _unref(1) {
  }
  void cancel() {
    _unref = 0;
  }
  inline ~unref_if_return() {
    if (_unref && _ref) { lua_unref(_L, _ref); }
  }
};

#define lua_auto_unref  unref_if_return

/***********************************************************************************/

class revert_if_return final {
  revert_if_return(const revert_if_return&) = delete;
  int _top;
  lua_State* _L;

public:
  inline ~revert_if_return() {
    lua_settop(_L, _top);
  }
  inline revert_if_return(lua_State* L)
    : _L(L)
    , _top(lua_gettop(L)) {
  }
  inline int top() const { return _top; }
};

#define lua_auto_revert revert_if_return

/***********************************************************************************/

template <typename _Ty, typename ...Args>
_Ty* newuserdata(lua_State* L, const char* name, Args... args) {
  void* userdata = lua_newuserdata(L, sizeof(_Ty));
  luaL_getmetatable(L, name);
  lua_setmetatable(L, -2);
  return new (userdata) _Ty(args...);
}

template <typename _Ty>
_Ty* checkudata(lua_State* L, int index, const char* name) {
  return (_Ty*)luaL_checkudata(L, index, name);
}

inline int newmetatable(lua_State* L, const char* name, const luaL_Reg methods[]) {
  if (!luaL_newmetatable(L, name)) {
    return 0;
  }
  /* define methods */
  if (methods) {
    luaL_setfuncs(L, methods, 0);
  }
  /* define metamethods */
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);

  lua_pushliteral(L, "__metatable");
  lua_pushliteral(L, "you're not allowed to get this metatable");
  lua_settable(L, -3);
  return 1;
}

/***********************************************************************************/

#endif //LUASET_H
