

#include "../skynet.h"
#include "lua_timer.h"

/********************************************************************************/

struct class_timer final {
  inline class_timer()
    : _timer(*lua_service()) {
  }
  int on_timer(lua_State* L, size_t timeout, int handler) {
    _timer.expires_after(
      std::chrono::milliseconds(timeout)
    );
    _timer.async_wait([=](const error_code& ec) {
      lua_State* L = lua_local();
      lua_auto_revert revert(L);
      if (ec) {
        return;
      }
      if (lua_pushref(L, handler) != LUA_TFUNCTION) {
        return;
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        lua_ferror("%s\n", luaL_checkstring(L, -1));
      }
      auto expires = lua_tointeger(L, -1);
      if (expires <= 0) {
        return;
      }
      on_timer(L, (expires ? expires : timeout), handler);
    });
    return 0;
  }
  int _handler = 0;
  steady_timer _timer;

public:
  inline static const char* name() {
    return "skynet timer";
  }
  inline static class_timer* __this(lua_State* L) {
    return checkudata<class_timer>(L, 1, name());
  }
  inline static int __gc(lua_State* L) {
    auto self = __this(L);
#ifdef LUA_DEBUG
    lua_ftrace("DEBUG: %s will gc\n", name());
#endif
    cancel(L);
    self->~class_timer();
    return 0;
  }
  static int cancel(lua_State* L) {
    auto self = __this(L);
    if (self->_handler == 0) {
      return 0;
    }
    self->_timer.cancel();
    lua_unref(L, self->_handler);
    return self->_handler = 0;
  }
  static int expires(lua_State* L) {
    cancel(L);
    auto self = __this(L);
    size_t timeout = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    self->_handler = lua_ref(L, 3);
    return self->on_timer(L, timeout, self->_handler);
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
  static int create(lua_State* L) {
    auto self = newuserdata<class_timer>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushliteral(L, "no memory");
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
    return new_module(L, "os", methods);
  }
};

/********************************************************************************/

SKYNET_API int luaopen_timer(lua_State* L) {
  return class_timer::open_library(L);
}

/********************************************************************************/
