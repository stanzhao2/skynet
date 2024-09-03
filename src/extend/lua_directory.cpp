

#include "lua_list.h"

/********************************************************************************/

struct directory final {
  inline static const char* name() {
    return "lua directory";
  }
  inline static directory* __this(lua_State* L) {
    return checkudata<directory>(L, 1, name());
  }
  static int __gc(lua_State* L) {
    auto self = __this(L);
    self->close(L);
    self->~directory();
    return 0;
  }
  static int close(lua_State* L) {
    auto self = __this(L);
    if (!self->closed) {
      tinydir_close(&self->tdir);
    }
    self->closed = true;
    return 0;
  }
  inline static int pairs(lua_State* L) {
    auto self = __this(L);
    lua_pushlightuserdata(L, self);
    lua_pushcclosure(L, iterator, 1);
    return 1;
  }
  inline static int iterator(lua_State* L) {
    auto self = (directory*)lua_touserdata(L, lua_upvalueindex(1));
    while (!self->closed) {
      if (!self->tdir.has_next) {
        break;
      }
      tinydir_file file;
      tinydir_readfile(&self->tdir, &file);
      tinydir_next(&self->tdir);
      auto filename = UTF8(file.name);
      if (filename != "." && filename != "..") {
        lua_pushlstring(L, filename.c_str(), filename.size());
        lua_pushboolean(L, file.is_dir);
        return 2;
      }
    }
    return 0;
  }
  static int open(lua_State* L) {
    const char* dir = luaL_optstring(L, 1, "." LUA_DIRSEP);
    std::string path = UTF8(dir);
    auto self = newuserdata<directory>(L, name());
    if (tinydir_open(&self->tdir, path.c_str()) < 0) {
      lua_pushnil(L);
    }
    return 1;
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
  tinydir_dir tdir;
  bool closed = false;
};

/********************************************************************************/

SKYNET_API int luaopen_directory(lua_State* L) {
  return directory::open_library(L);
}

/********************************************************************************/
