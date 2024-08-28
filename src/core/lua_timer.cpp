

#include "lua_timer.h"

/********************************************************************************/

struct class_timer final {
  inline class_timer(lua_State* lua)
    : timer(*lua_service())
    , L(lua) {
  }
  int __expires() {
    size_t timeout = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    return __expires(timeout, lua_ref(L, 3));
  }
  int __cancel() {
    timer.cancel();
    return 0;
  }
  int __expires(size_t timeout, int handler) {
    timer.expires_after(
      std::chrono::milliseconds(timeout)
    );
    timer.async_wait([=](const error_code& ec) {
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
      __expires(timeout, handler);
    });
    return 0;
  }
  lua_State* L;
  steady_timer timer;

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
    return __this(L)->__expires();
  }
  static int cancel(lua_State* L) {
    return __this(L)->__cancel();
  }
  static int create(lua_State* L) {
    auto self = newuserdata<class_timer>(L, name(), L);
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
      { "timer",  create   }, /* os.timer() */
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
