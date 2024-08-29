

#include "lua_list.h"

/********************************************************************************/

struct class_list final {
  inline class_list()
    : iter(data.end()) {
  }
  inline ~class_list() {
    __clear(nullptr);
  }
  int __clear(lua_State* L) {
    for (auto iter = data.begin(); iter != data.end(); ++iter) {
      lua_unref(L, *iter);
    }
    data.clear();
    return 0;
  }
  int __size(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)data.size());
    return 1;
  }
  int __empty(lua_State* L) {
    lua_pushboolean(L, data.empty() ? 1 : 0);
    return 1;
  }
  int __erase(lua_State* L) {
    if (iter != data.end()) {
      auto i = --iter;
      lua_unref(L, *i);
      iter++;
      data.erase(i);
    }
    else if (!data.empty()) {
      lua_unref(L, data.back());
      data.pop_back();
    }
    return 0;
  }
  int __reverse(lua_State* L) {
    data.reverse();
    return 0;
  }
  int __front(lua_State* L) {
    if (data.empty()) {
      lua_pushnil(L);
    }
    else {
      lua_pushref(L, data.front());
    }
    return 1;
  }
  int __back(lua_State* L) {
    if (data.empty()) {
      lua_pushnil(L);
    }
    else {
      lua_pushref(L, data.back());
    }
    return 1;
  }
  int __pop_front(lua_State* L) {
    if (!data.empty()) {
      int front = data.front();
      data.pop_front();
      lua_unref(L, front);
    }
    return 0;
  }
  int __pop_back(lua_State* L){
    if (!data.empty()) {
      if (iter != data.end()) {
        --iter;
      }
      int back = data.back();
      data.pop_back();
      lua_unref(L, back);
      if (data.empty()) {
        iter = data.end();
      }
      else if (iter != data.end()) {
        ++iter;
      }
    }
    return 0;
  }
  int __push_front(lua_State* L){
    luaL_checkany(L, 2);
    data.push_front(lua_ref(L, 2));
    return 0;
  }
  int __push_back(lua_State* L){
    luaL_checkany(L, 2);
    data.push_back(lua_ref(L, 2));
    return 0;
  }
  int __iterator(lua_State* L){
    if (iter == data.end()) {
      return 0;
    }
    lua_pushref(L, *(iter++));
    return 1;
  }
  int __pairs(lua_State* L) {
    iter = data.begin();
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, iterator, 1);
    return 1;
  }
  std::list<int> data;
  std::list<int>::iterator iter;

public:
  static const char* name() {
    return "lua list";
  }
  static class_list* __this(lua_State* L) {
    return checkudata<class_list>(L, 1, name());
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",         __gc        },
      { "__pairs",      pairs       },
      { "__len",        size        },
      { "size",         size        },
      { "empty",        empty       },
      { "clear",        clear       },
      { "erase",        erase       },
      { "front",        front       },
      { "back",         back        },
      { "reverse",      reverse     },
      { "push_front",   push_front  },
      { "pop_front",    pop_front   },
      { "push_back",    push_back   },
      { "pop_back",     pop_back    },
      { NULL,           NULL        }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int clear(lua_State* L) {
    return __this(L)->__clear(L);
  }
  static int __gc(lua_State* L) {
    __this(L)->~class_list();
    return 0;
  }
  static int size(lua_State* L) {
    return __this(L)->__size(L);
  }
  static int empty(lua_State* L) {
    return __this(L)->__empty(L);
  }
  static int erase(lua_State* L) {
    return __this(L)->__erase(L);
  }
  static int reverse(lua_State* L) {
    return __this(L)->__reverse(L);
  }
  static int front(lua_State* L) {
    return __this(L)->__front(L);
  }
  static int back(lua_State* L) {
    return __this(L)->__back(L);
  }
  static int pop_front(lua_State* L) {
    return __this(L)->__pop_front(L);
  }
  static int pop_back(lua_State* L) {
    return __this(L)->__pop_back(L);
  }
  static int push_front(lua_State* L) {
    return __this(L)->__push_front(L);
  }
  static int push_back(lua_State* L) {
    return __this(L)->__push_back(L);
  }
  static int iterator(lua_State* L) {
    auto self = (class_list*)lua_touserdata(L, lua_upvalueindex(1));
    return self->__iterator(L);
  }
  static int pairs(lua_State* L) {
    return __this(L)->__pairs(L);
  }
  static int create(lua_State* L) {
    auto self = newuserdata<class_list>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushstring(L, "no memory");
      return 2;
    }
    return 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    bool need_pop = true;
    lua_getglobal(L, "std");
    if (lua_type(L, -1) != LUA_TTABLE) {
      lua_pop(L, 1);
      lua_newtable(L);
      need_pop = false;
    }
    const luaL_Reg methods[] = {
      { "list",   create    },
      { NULL,     NULL      },
    };
    luaL_setfuncs(L, methods, 0);
    need_pop ? lua_pop(L, 1) : lua_setglobal(L, "std");
    return 0;
  }
};

/********************************************************************************/

SKYNET_API int luaopen_list(lua_State* L) {
  return class_list::open_library(L);
}

/********************************************************************************/
