

#include "lua_pload.h"
#include "lua_wrap.h"
#include "lua_print.h"
#include "lua_dofile.h"

#define nameof_pload "lua thread"

/********************************************************************************/

/* reference of context for pload thread */
static thread_local pload_context *thread_context = nullptr;

static int luac_mt_close(lua_State* L) {
  pload_context* context = checkudata<pload_context>(L, 1, nameof_pload);
  if (!context || !context->thread->joinable()) {
    return 0;
  }
  context->ios->stop();
  context->thread->join();
  context->state = pload_state::exited;
  return 0;
}

static int luac_mt_gc(lua_State* L) {
  pload_context* context = checkudata<pload_context>(L, 1, nameof_pload);
  int result = luac_mt_close(L);
  context->~pload_context();
  return result;
}

static int luac_mt_id(lua_State* L) {
  pload_context* context = checkudata<pload_context>(L, 1, nameof_pload);
  lua_pushinteger(L, context->ios->id());
  return 1;
}

static int luac_mt_state(lua_State* L) {
  pload_context* context = checkudata<pload_context>(L, 1, nameof_pload);
  switch (context->state) {
  case pload_state::successfully:
    lua_pushstring(L, "active");
    break;
  case pload_state::error:
    lua_pushstring(L, "error");
    break;
  case pload_state::exited:
    lua_pushstring(L, "dead");
    break;
  }
  return 1;
}

/********************************************************************************/

static void clonepath(lua_State* from, lua_State* to, const char* name) {
  int topfrom = lua_gettop(from);
  int topto   = lua_gettop(to);

  lua_getglobal(from, LUA_LOADLIBNAME);
  lua_getfield (from, -1, name);
  const char* path = luaL_checkstring(from, -1);

  lua_getglobal (to, LUA_LOADLIBNAME);
  lua_pushstring(to, path);
  lua_setfield  (to, -2, name);

  lua_settop(to, topto);
  lua_settop(from, topfrom);
}

/* current microservice thread */
static void pload_thread(lua_State* L, pload_context* context) {
  context->L = lua_local();
  if (!context->L) {
    context->state = pload_state::error;
    context->error = "no memory";
    return;
  }
  context->ios = lua_service();
  clonepath(L, context->L, "path");
  clonepath(L, context->L, "cpath");

  int argc = 1;
  L = context->L;
  lua_pushcfunction(L, lua_dofile);
  lua_pushstring(L, context->name.c_str());

  if (!context->argv.empty()) {
    lua_pushlstring(L, context->argv.c_str(), context->argv.size());
    argc += lua_unwrap(L);
  }

  thread_context = context;
  int status = lua_pcall(L, argc, 0);
  if (status != LUA_OK) {
    context->error = luaL_checkstring(L, -1);
    context->state = pload_state::error;
    lua_ferror("%s\n", context->error.c_str());
  }
  else {
    context->error = "exit";
    context->state = pload_state::exited;
  }
  lua_close(L);
}

/* start a lua microservice */
static int luac_pload(lua_State* L) {
  size_t size;
  int argc = lua_gettop(L) - 1;
  if (argc < 0) {
    luaL_error(L, "no name");
  }
  const char* argv = "";
  const char* name = luaL_checkstring(L, 1);
  if (*name == 0) {
    luaL_error(L, "no name");
  }
  if (argc > 0) {
    lua_wrap(L, argc);
    argv = luaL_checklstring(L, -1, &size);
  }
  pload_context* context = newuserdata<pload_context>(L, nameof_pload);
  if (context == NULL) {
    luaL_error(L, "no memory");
    return 0;
  }
  context->state  = pload_state::pending;
  context->L      = 0;
  context->ios    = 0;
  context->name   = name;
  context->argv   = argv;
  context->thread = std::make_shared<std::thread>(std::bind(pload_thread, L, context));

  while (context->state == pload_state::pending) {
    if (context->lock.wait_for(100)) {
      context->state = pload_state::successfully;
      break;
    }
  }
  if (context->state == pload_state::successfully) {
    lua_pushboolean(L, 1);
    lua_rotate(L, -2, 1);
  }
  else {
    lua_pushboolean(L, 0);
    lua_pushlstring(L, context->error.c_str(), context->error.size());
    if (context->thread->joinable()) {
      context->thread->join();
    }
  }
  return 2;
}

static void init_metatable(lua_State* L) {
  const luaL_Reg methods[] = {
    { "__gc",       luac_mt_gc     },
    { "state",      luac_mt_state  },
    { "id",         luac_mt_id     },
    { "close",      luac_mt_close  },
    { NULL,         NULL           }
  };
  newmetatable(L, nameof_pload, methods);
  lua_pop(L, 1);
}

/********************************************************************************/

/* get context of current thread */
SKYNET_API pload_context* local_context(lua_State* L) {
  return thread_context;
}

SKYNET_API int luaopen_pload(lua_State* L) {
  init_metatable(L);
  const luaL_Reg methods[] = {
    { "pload",      luac_pload    },
    { NULL,         NULL          }
  };
  lua_getglobal(L, LUA_GNAME);
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return 0;
}

/********************************************************************************/
