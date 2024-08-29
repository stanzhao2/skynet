

#include "lua_timer.h"

/********************************************************************************/

struct class_timer final {
  inline class_timer()
    : _timer(*lua_service()) {
  }
  int __expires(lua_State* L) {
    size_t timeout = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    return __expires(L, timeout, lua_ref(L, 3));
  }
  int __cancel(lua_State* L) {
    _timer.cancel();
    return 0;
  }
  int __expires(lua_State* L, size_t timeout, int handler) {
    _timer.expires_after(
      std::chrono::milliseconds(timeout)
    );
    _timer.async_wait([=](const error_code& ec) {
      lua_auto_revert revert(L);
      lua_auto_unref  unref(L, handler);
      if (ec) {
        return;
      }
      lua_pushref(L, handler);
      if (lua_pcall(L, 0, 1) != LUA_OK) {
        lua_ferror("%s\n", luaL_checkstring(L, -1));
      }
      int what = lua_type(L, -1);
      if (what == LUA_TBOOLEAN) {
        if (lua_toboolean(L, -1) == 0) {
          return;
        }
      }
      unref.cancel();
      __expires(L, timeout, handler);
    });
    return 0;
  }
  std::string  _name;
  steady_timer _timer;

public:
  static const char* name() {
    return "lua timer";
  }
  static class_timer* __this(lua_State* L) {
    return checkudata<class_timer>(L, 1, name());
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",      __gc        },
      { "cancel",    cancel      },
      { "expires",   expires     },
      { NULL,        NULL        }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int __gc(lua_State* L) {
    __this(L)->~class_timer();
    return 0;
  }
  static int expires(lua_State* L) {
    return __this(L)->__expires(L);
  }
  static int cancel(lua_State* L) {
    return __this(L)->__cancel(L);
  }
  static int create(lua_State* L) {
    auto self = newuserdata<class_timer>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushstring(L, "no memory");
      return 2;
    }
    self->_name = luaL_optstring(L, 1, "no name");
    return 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    const luaL_Reg methods[] = {
      { "_timer",  create   }, /* os._timer() */
      { NULL,     NULL     }
    };
    lua_getglobal(L, "os");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1); /* pop '_G' from stack */
    return 0;
  }
};

/********************************************************************************/

SKYNET_API int luaopen_timer(lua_State* L) {
  return class_timer::open_library(L);
}

/********************************************************************************/
