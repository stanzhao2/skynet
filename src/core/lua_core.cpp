

#include "../skynet_lua.h"
#include "lua_bind.h"
#include "lua_dofile.h"
#include "lua_path.h"
#include "lua_pcall.h"
#include "lua_pload.h"
#include "lua_print.h"
#include "lua_require.h"
#include "lua_wrap.h"
#include "lua_timer.h"
#include "lua_socket.h"

/********************************************************************************/

static const lua_CFunction core_modules[] = {
  luaopen_require,      /* require        */
  luaopen_print,        /* print, trace, error, throw */
  luaopen_path,         /* path, cpath    */
  luaopen_wrap,         /* wrap, unwrap   */
  luaopen_pcall,        /* pcall, xpcall  */
  luaopen_bind,         /* bind           */
  luaopen_pload,        /* pload          */
  luaopen_timer,        /* os.timer       */
  luaopen_socket,       /* io.socket      */
  NULL
};

static int openlibs(lua_State* L, const lua_CFunction f[]) {
  while (f && *f) {
    lua_pushcfunction(L, *f++);
    if (lua_pcall(L, 0, 0) != LUA_OK) {
      lua_error(L);
    }
  }
  return 0;
}

/********************************************************************************/

struct directory final {
  directory(lua_State* lua)
    : L(lua) {
  }
  int __close() {
    if (!closed) {
      tinydir_close(&tdir);
    }
    closed = true;
    return 0;
  }
  int __iterator() {
    while (!closed && tdir.has_next) {
      tinydir_file file;
      tinydir_readfile(&tdir, &file);
      tinydir_next(&tdir);
      auto filename = UTF8(file.name);
      if (filename != "." && filename != "..") {
        lua_pushlstring(L, filename.c_str(), filename.size());
        lua_pushboolean(L, file.is_dir);
        return 2;
      }
    }
    return 0;
  }
  int __pairs(){
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, iterator, 1);
    return 1;
  }
  lua_State* L;
  tinydir_dir tdir;
  bool closed = false;

public:
  static const char* name() {
    return "lua folder";
  }
  static directory* __this(lua_State* L) {
    return checkudata<directory>(L, 1, name());
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",     __gc    },
      { "__pairs",  pairs   },
      { "close",    close   },
      { NULL,       NULL    }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int close(lua_State* L) {
    return __this(L)->__close();
  }
  static int __gc(lua_State* L) {
    __this(L)->~directory();
    return 0;
  }
  static int pairs(lua_State* L) {
    return __this(L)->__pairs();
  }
  static int iterator(lua_State* L) {
    auto self = (directory*)lua_touserdata(L, lua_upvalueindex(1));
    return self->__iterator();
  }
  static int open(lua_State* L) {
    const char* dir = luaL_optstring(L, 1, "." LUA_DIRSEP);
    std::string path = UTF8(dir);
    auto self = newuserdata<directory>(L, name(), L);
    if (tinydir_open(&self->tdir, path.c_str()) < 0) {
      lua_pushnil(L);
    }
    return 1;
  }
  static int make(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    auto ok = make_dir(name);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    const luaL_Reg methods[] = {
      { "mkdir",      make  },
      { "opendir",    open  },
      { NULL,         NULL  }
    };
    lua_getglobal(L, "os");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1); /* pop 'os' from stack */
    return 0;
  }
};

/********************************************************************************/

struct global final {
  static int check_access(lua_State* L) {
    lua_Debug ar;
    if (lua_getstack(L, 1, &ar) == 0) {
      lua_rawset(L, 1);
      return 0;
    }
    if (lua_getinfo(L, "Sln", &ar) && ar.currentline < 0) {
      lua_rawset(L, 1);
      return 0;
    }
    const char* name = luaL_checkstring(L, -2);
    if (lua_isfunction(L, -1)) {
      if (strcmp(name, nameof_main) == 0) {
        lua_rawset(L, 1);
        return 0;
      }
    }
    return luaL_error(L, "Cannot use global variables: %s", name);
  }
  static int open_library(lua_State* L) {
    lua_getglobal(L, LUA_GNAME);
    lua_getfield(L, -1, "__newindex");
    if (lua_isfunction(L, -1)) {
      lua_pop(L, 2);
      return 0;
    }
    lua_pop(L, 1);
    luaL_newmetatable(L, "placeholders");
    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, check_access);

    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
    return 0;
  }
};

/********************************************************************************/

static typeof<io::service> main_service;
static const size_t init_clock = steady_clock();

static int os_clock(lua_State *L) {
  const char* opt = luaL_optstring(L, 1, "s");
  auto now = steady_clock() - init_clock;
  if (strcmp(opt, "ms") == 0) {
    lua_pushinteger(L, (lua_Integer)now);
  }else{
    lua_pushnumber(L, (lua_Number)(now) / 1000.0f);
  }
  return 1;
}

static int os_stopped(lua_State* L) {
  auto ios = skynet_service();
  lua_pushboolean(L, ios->stopped());
  return 1;
}

static int os_id(lua_State* L) {
  auto ios = skynet_service();
  lua_pushinteger(L, ios->id());
  return 1;
}

static int os_name(lua_State* L) {
  lua_getglobal(L, "progname");
  return 1;
}

static int os_processors(lua_State* L) {
  lua_pushinteger(L, (lua_Integer)cpu_hardware());
  return 1;
}

/* exit the current microservice */
static int os_stop(lua_State* L) {
  auto ios = skynet_service();
  ios->stop();
  return 0;
}

/* force exit from the current process */
static int os_exit(lua_State* L) {
  main_service->stop();
  return 0;
}

static int os_restart(lua_State* L) {
  auto ios = skynet_service();
  ios->restart();
  return 0;
}

static int os_post(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  int argc    = lua_gettop(L);
  int handler = lua_ref(L, 1);

  std::vector<int> params;
  for (int i = 2; i <= argc; i++) {
    params.push_back(lua_ref(L, i));
  }

  auto executor = skynet_service();
  executor->post([handler, params]() {
    lua_State* L = skynet_local();
    lua_auto_revert revert(L);
    lua_auto_unref  unref(L, handler);
    if (lua_pushref(L, handler) != LUA_TFUNCTION) {
      return;
    }
    size_t argc = params.size();
    for (size_t i = 0; i < argc; i++) {
      lua_pushref(L, params[i]);
      lua_unref(L, params[i]);
    }
    if (lua_pcall(L, (int)argc, 0) != LUA_OK) {
      lua_ferror("%s\n", luaL_checkstring(L, -1));
    }
  });
  return 0;
}

static int os_wait(lua_State* L) {
  pload_context* context = local_context(L);
  if (context) {
    if (context->state == pload_state::pending) {
      context->lock.signal();
    }
  }
  size_t count = 0;
  auto ios = skynet_service();
  auto expires = luaL_optinteger(L, 1, -1);
  if (expires < 0) {
    count = ios->run();
  }
  else if (expires > 0) {
    count = ios->run_for(std::chrono::milliseconds(expires));
  }
  else {
    count = ios->poll();
  }
  lua_pushinteger(L, count);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_core(lua_State* L) {
  if (!main_service) {
    main_service = io::service::local();
  }
  global::open_library(L);
  directory::open_library(L);
  const luaL_Reg methods[] = {
    { "clock",      os_clock      },
    { "wait",       os_wait       },
    { "id",         os_id         },
    { "name",       os_name       },
    { "processors", os_processors },
    { "exit",       os_exit       },
    { "stop",       os_stop       },
    { "stopped",    os_stopped    },
    { "post",       os_post       },
    { "restart",    os_restart    },
    { "mkdir",      directory::make },
    { "opendir",    directory::open },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */
  return openlibs(L, core_modules);
}

/********************************************************************************/
