

#include "lua_storage.h"
#include "../core/lua_wrap.h"
#include "../core/lua_pcall.h"
#include "../core/lua_print.h"

/*******************************************************************************/

static std::recursive_mutex _mutex;
static std::map<std::string, std::string> _storage;

struct pcall_context {
  int top;
  std::string key;
  std::string old_value;
};

static int finishpcall(lua_State *L, int status, lua_KContext extra) {
  if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    lua_error(L);
  }
  if (lua_toboolean(L, -1) == 0) {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    return 2;
  }
  auto ctx = (pcall_context*)extra;
  int  top = ctx->top;
  auto key = ctx->key;
  auto old_value = ctx->old_value;

  lua_settop(L, top);
  lua_wrap(L, top - 2);
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);
  _storage[key] = std::move(std::string(data, size));
  if (old_value.empty()) {
    lua_pushboolean(L, 1);
    lua_pushnil(L);
    delete ctx;
    return 2;
  }
  lua_pushboolean(L, 1);
  lua_pushlstring(L, old_value.c_str(), old_value.size());
  delete ctx;
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
  std::string old_value;
  std::string key = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  int top = lua_gettop(L);
  if (top < 3) {
    luaL_error(L, "no values");
  }
  int argc = top - 2;
  lua_pushvalue(L, 2); /* push function to stack */
  for (int i = 3; i <= top; i++) {
    lua_pushvalue(L, i); /* push params to stack */
  }
  std::unique_lock<std::recursive_mutex> lock(_mutex);
  auto iter = _storage.find(key);
  if (iter != _storage.end()) {
    old_value = iter->second;
  }
  if (!old_value.empty()) {
    lua_pushlstring(L, old_value.c_str(), old_value.size());
    argc += lua_unwrap(L);
  }
  pcall_context* ctx = new pcall_context();
  if (!ctx) {
    lua_pushboolean(L, 0);
    lua_pushnil(L);
    return 2;
  }
  ctx->key = key;
  ctx->top = top;
  ctx->old_value = old_value;
  int status = lua_pcall_k(L, argc, 1, ctx, finishpcall);
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
  lua_newtable(L);
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
  luaL_setfuncs(L, methods, 0);
  lua_setglobal(L, "storage");
  return 0;
}

/*******************************************************************************/
