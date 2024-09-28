

#include "lua_list.h"

/********************************************************************************/

struct class_list final {
  inline class_list()
    : iter(data.end()) {
  }
  inline static const char* name() {
    return "lua list";
  }
  inline static class_list* __this(lua_State* L) {
    return checkudata<class_list>(L, 1, name());
  }
  static int clear(lua_State* L) {
    auto self = __this(L);
    auto ppos = self->data.begin();
    for (; ppos != self->data.end(); ++ppos) {
      lua_unref(L, *ppos);
    }
    self->data.clear();
    return 0;
  }
  static int __gc(lua_State* L) {
    clear(L);
    __this(L)->~class_list();
    return 0;
  }
  static int size(lua_State* L) {
    auto self = __this(L);
    lua_pushinteger(L, (lua_Integer)self->data.size());
    return 1;
  }
  static int empty(lua_State* L) {
    auto self = __this(L);
    lua_pushboolean(L, self->data.empty() ? 1 : 0);
    return 1;
  }
  static int erase(lua_State* L) {
    auto self = __this(L);
    if (self->iter != self->data.end()) {
      auto i = --self->iter;
      lua_unref(L, *i);
      self->iter++;
      self->data.erase(i);
    }
    else if (!self->data.empty()) {
      lua_unref(L, self->data.back());
      self->data.pop_back();
    }
    return 0;
  }
  static int reverse(lua_State* L) {
    auto self = __this(L);
    self->data.reverse();
    return 0;
  }
  static int front(lua_State* L) {
    auto self = __this(L);
    if (self->data.empty()) {
      lua_pushnil(L);
    }
    else {
      lua_pushref(L, self->data.front());
    }
    return 1;
  }
  static int back(lua_State* L) {
    auto self = __this(L);
    if (self->data.empty()) {
      lua_pushnil(L);
    }
    else {
      lua_pushref(L, self->data.back());
    }
    return 1;
  }
  static int pop_front(lua_State* L) {
    auto self = __this(L);
    if (!self->data.empty()) {
      int ref = self->data.front();
      self->data.pop_front();
      lua_unref(L, ref);
    }
    return 0;
  }
  static int pop_back(lua_State* L) {
    auto self = __this(L);
    if (!self->data.empty()) {
      if (self->iter != self->data.end()) {
        --self->iter;
      }
      int back = self->data.back();
      self->data.pop_back();
      lua_unref(L, back);
      if (self->data.empty()) {
        self->iter = self->data.end();
      }
      else if (self->iter != self->data.end()) {
        ++self->iter;
      }
    }
    return 0;
  }
  static int push_front(lua_State* L) {
    auto self = __this(L);
    luaL_checkany(L, 2);
    self->data.push_front(lua_ref(L, 2));
    return 0;
  }
  static int push_back(lua_State* L) {
    auto self = __this(L);
    luaL_checkany(L, 2);
    self->data.push_back(lua_ref(L, 2));
    return 0;
  }
  static int iterator(lua_State* L) {
    auto self = (class_list*)lua_touserdata(L, lua_upvalueindex(1));
    if (self->iter == self->data.end()) {
      return 0;
    }
    lua_pushref(L, *(self->iter++));
    return 1;
  }
  static int pairs(lua_State* L) {
    auto self = __this(L);
    self->iter = self->data.begin();
    lua_pushlightuserdata(L, self);
    lua_pushcclosure(L, iterator, 1);
    return 1;
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
    const luaL_Reg methods[] = {
      { "list",   create    },
      { NULL,     NULL      },
    };
    return new_module(L, "std", methods);
  }
  std::list<int> data;
  std::list<int>::iterator iter;
};

/********************************************************************************/

SKYNET_API int luaopen_list(lua_State* L) {
  return class_list::open_library(L);
}

/********************************************************************************/
