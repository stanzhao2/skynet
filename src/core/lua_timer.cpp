

#include "../skynet.h"
#include "lua_timer.h"

/********************************************************************************/

struct class_timer final {
  inline class_timer()
    : _timer(*lua_service()) {
  }
  int on_timer(lua_State* L, size_t timeout, int handler, std::shared_ptr<bool> pending) {
    _timer.expires_after(
      std::chrono::milliseconds(timeout)
    );
    _timer.async_wait([=](const error_code& ec) {
      lua_State* L = lua_local();
      lua_auto_revert revert(L);
      lua_auto_unref  unref(L, handler);
      if (ec || !*pending) {
        return;
      }
      lua_pushref(L, handler);
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        lua_ferror("%s\n", luaL_checkstring(L, -1));
      }
      size_t expires = lua_tointeger(L, -1);
      if (expires == 0) {
        return;
      }
      unref.cancel();
      on_timer(L, (expires ? expires : timeout), handler, pending);
    });
    return 0;
  }
  steady_timer _timer;
  std::shared_ptr<bool> _pending;

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
    self->_timer.cancel();
    if (self->_pending) {
      *self->_pending = false;
      self->_pending.reset();
    }
    return 0;
  }
  static int expires(lua_State* L) {
    auto self = __this(L);
    size_t timeout = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    cancel(L);
    auto handler = lua_ref(L, 3);
    self->_pending = std::make_shared<bool>(true);
    return self->on_timer(L, timeout, handler, self->_pending);
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
