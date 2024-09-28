

#include "lua_storage.h"
#include "../core/lua_wrap.h"
#include "../core/lua_pcall.h"
#include "../core/lua_print.h"

/*******************************************************************************/

static std::recursive_mutex _mutex;
static std::map<std::string, std::string> _storage;

struct pcall_context {
  template <typename _Mutex>
  inline pcall_context(_Mutex& mutex)
    : lock(mutex) {
  }
  int top = 0;
  std::string key;
  std::string old_value;
  std::unique_lock<std::recursive_mutex> lock;
};

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  auto context = (pcall_context*)extra;
  class __delete {
    pcall_context* _ctx;
  public:
    inline  __delete(pcall_context* ctx) : _ctx(ctx) {}
    inline ~__delete() { delete _ctx; }
  } auto_delete(context);

  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_error(L);
  }
  int top = lua_gettop(L);
  if (top < context->top) {
    luaL_error(L, "no result");
  }
  lua_wrap(L, top - context->top + 1);
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);

  std::string key = std::move(context->key);
  std::string old_value = std::move(context->old_value);
  _storage[key] = std::move(std::string(data, size));
  if (old_value.empty()) {
    lua_pushboolean(L, 1);
    lua_pushnil(L);
    return 2;
  }
  lua_pushboolean(L, 1);
  lua_pushlstring(L, old_value.c_str(), old_value.size());
  return lua_unwrap(L) + 1;
}

/*******************************************************************************/

static int luac_set(lua_State* L) {
  std::string old_value;
  std::string key = luaL_checkstring(L, 1);
  int top = lua_gettop(L);
  if (top < 2) {
    luaL_error(L, "no value");
  }
  lua_wrap(L, lua_gettop(L) - 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter != _storage.end()) {
    old_value = iter->second;
  }
  _storage[key] = luaL_checkstring(L, -1);
  if (old_value.empty()) {
    lua_pushnil(L);
    return 1;
  }
  lua_pushlstring(L, old_value.c_str(), old_value.size());
  return lua_unwrap(L);
}

static int luac_set_if(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  pcall_context* ctx = new pcall_context(_mutex);
  if (!ctx) {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    return 2;
  }
  ctx->top = lua_gettop(L);
  ctx->key = name;
  auto iter = _storage.find(ctx->key);
  if (iter != _storage.end()) {
    ctx->old_value = iter->second;
  }
  int count = 0;
  if (!ctx->old_value.empty()) {
    lua_pushlstring(L, ctx->old_value.c_str(), ctx->old_value.size());
    count = lua_unwrap(L);
  }
  int status = lua_pcall_k(L, count, LUA_MULTRET, ctx, finishpcall);
  return finishpcall(L, status, (lua_KContext)ctx);
}

static int luac_get(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);

  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushnil(L);
    return 1;
  }
  std::string value = iter->second;
  lua_pushlstring(L, value.c_str(), value.size());
  return lua_unwrap(L);
}

static int luac_exist(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushboolean(L, 0);
  }
  else {
    lua_pushboolean(L, 1);
  }
  return 1;
}

static int luac_erase(lua_State* L) {
  std::string key = luaL_checkstring(L, 1);
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter == _storage.end()) {
    lua_pushnil(L);
    return 1;
  }
  std::string value = iter->second;
  _storage.erase(iter);
  lua_pushlstring(L, value.c_str(), value.size());
  return lua_unwrap(L);
}

static int luac_size(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  lua_pushinteger(L, (lua_Integer)_storage.size());
  return 1;
}

static int luac_empty(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  lua_pushboolean(L, _storage.empty() ? 1 : 0);
  return 1;
}

static int luac_clear(lua_State* L) {
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  _storage.clear();
  return 0;
}

/*******************************************************************************/

SKYNET_API int luaopen_storage(lua_State* L) {
  const luaL_Reg methods[] = {
    { "exist",    luac_exist  },
    { "size",     luac_size   },
    { "empty",    luac_empty  },
    { "set",      luac_set    },
    { "set_if",   luac_set_if },
    { "get",      luac_get    },
    { "erase",    luac_erase  },
    { "clear",    luac_clear  },
    { NULL,       NULL        }
  };
  return new_module(L, "storage", methods);
}

/*******************************************************************************/
