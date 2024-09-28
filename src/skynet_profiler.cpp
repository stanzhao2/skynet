

#include "skynet_lua.h"
#include "skynet_profiler.h"

/********************************************************************************/

static void mark_object(
  lua_State *L, lua_State *dL, const void * parent, const char * desc
);

#define UNKNOWN		0
#define TABLE			1
#define FUNCTION	2
#define THREAD		3
#define USERDATA	4
#define MARK			5

/********************************************************************************/

static bool is_marked(lua_State* dL, const void* p) {
  lua_rawgetp(dL, MARK, p);
  if (lua_isnil(dL, -1)) {    /* if not marked */
    lua_pop(dL, 1);           /* pop nil */
    lua_pushboolean(dL, 1);   /* push true */
    lua_rawsetp(dL, MARK, p); /* set marked: t[p] = true */
    return false;
  }
  lua_pop(dL, 1); /*pop true */
  return true;
}

static bool is_lightcfunction(lua_State *L, int i) {
  if (lua_iscfunction(L, i)) {
    if (lua_getupvalue(L, i, 1) == NULL) {
      return true; /* not have upvalue */
    }
    lua_pop(L, 1);
  }
  return false;
}

static int check_type(lua_State *L, int i) {
  switch (lua_type(L, i)) {
  case LUA_TTABLE:
    return TABLE;
  case LUA_TFUNCTION:
    if (is_lightcfunction(L, i)) break;
    return FUNCTION;
  case LUA_TTHREAD:
    return THREAD;
  case LUA_TUSERDATA:
    return USERDATA;
  }
  lua_pop(L, 1);
  return UNKNOWN;
}

static const void* read_object(lua_State *L, lua_State *dL, const void *parent, const char *desc) {
  int tidx = check_type(L, -1);
  if (tidx == UNKNOWN) {
    return NULL;
  }
  const void *pv = lua_topointer(L, -1);
  if (is_marked(dL, pv)) {
    lua_rawgetp(dL, tidx, pv);
    if (!lua_isnil(dL, -1)) {
      lua_pushstring(dL, desc);
      lua_rawsetp(dL, -2, parent);
    }
    lua_pop(dL, 1);
    lua_pop(L,  1);
    return NULL;
  }
  lua_newtable(dL);
  lua_pushstring(dL, desc);
  lua_rawsetp(dL, -2, parent);
  lua_rawsetp(dL, tidx, pv);
  return pv;
}

static const char* key_tostring(lua_State *L, int i, char * buffer, size_t size) {
  int type = lua_type(L, i);
  switch (type) {
  case LUA_TSTRING:
    return lua_tostring(L, i);
  case LUA_TNUMBER:
    snprintf(buffer, size, "[%lg]", lua_tonumber(L, i));
    break;
  case LUA_TBOOLEAN:
    snprintf(buffer, size, "[%s]", lua_toboolean(L, i) ? "true" : "false");
    break;
  case LUA_TNIL:
    snprintf(buffer, size, "[nil]");
    break;
  default:
    snprintf(buffer, size, "[%s:%p]", lua_typename(L, type), lua_topointer(L, i));
    break;
  }
  return buffer;
}

static void mark_table(lua_State *L, lua_State *dL, const void * parent, const char * desc) {
  const void * pv = read_object(L, dL, parent, desc);
  if (pv == NULL) {
    return;
  }
  bool weakk = false;
  bool weakv = false;
  if (lua_getmetatable(L, -1)) {
    lua_pushliteral(L, "__mode");
    lua_rawget(L, -2);
    if (lua_isstring(L,-1)) {
      const char *mode = lua_tostring(L, -1);
      if (strchr(mode, 'k')) {
        weakk = true;
      }
      if (strchr(mode, 'v')) {
        weakv = true;
      }
    }
    lua_pop(L,1);
    luaL_checkstack(L, LUA_MINSTACK, NULL);
    mark_table(L, dL, pv, "[metatable]");
  }
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (weakv) {
      lua_pop(L, 1);
    }
    else {
      char key[256];
      const char *desc = key_tostring(L, -2, key, sizeof(key));
      mark_object(L, dL, pv , desc);
    }
    if (!weakk) {
      lua_pushvalue(L, -1);
      mark_object(L, dL, pv , "[key]");
    }
  }
  lua_pop(L, 1);
}

static void mark_userdata(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
  const void * pv = read_object(L, dL, parent, desc);
  if (pv == NULL) {
    return;
  }
  if (lua_getmetatable(L, -1)) {
    mark_table(L, dL, pv, "[metatable]");
  }
  lua_getuservalue(L, -1);
  if (lua_isnil(L,-1)) {
    lua_pop(L,2);
  }
  else {
    mark_object(L, dL, pv, "[userdata]");
    lua_pop(L,1);
  }
}

static void mark_function(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
  const void * pv = read_object(L, dL, parent, desc);
  if (pv == NULL) {
    return;
  }
  for (int i = 1; ; i++) {
    const char *name = lua_getupvalue(L, -1, i);
    if (name == NULL) {
      break;
    }
    mark_object(L, dL, pv, name[0] ? name : "[upvalue]");
  }
  lua_pop(L,1); /* get upvalue only */
}

static void mark_thread(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
  const void * pv = read_object(L, dL, parent, desc);
  if (pv == NULL) {
    return;
  }
  int level = 0;
  lua_State *cL = lua_tothread(L, -1);
  if (cL == L) {
    level = 1;
  }
  else {
    int top = lua_gettop(cL);
    luaL_checkstack(cL, 1, NULL);
    char tmp[16];
    for (int i = 0; i < top; i++) {
      lua_pushvalue(cL, i + 1);
      sprintf(tmp, "[%d]", i + 1);
      mark_object(cL, dL, cL, tmp);
    }
  }
  lua_Debug ar;
  while (lua_getstack(cL, level, &ar)) {
    lua_getinfo(cL, "Sl", &ar);
    for (int j = 1; j > -1; j -= 2) {
      for (int i = j; ; i += j) {
        const char * name = lua_getlocal(cL, &ar, i);
        if (name == NULL) {
          break;
        }
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s:%s:%d", name, ar.short_src, ar.currentline);
        mark_object(cL, dL, pv, tmp);
      }
    }
    ++level;
  }
  lua_pop(L, 1);
}

static void mark_object(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
  luaL_checkstack(L, LUA_MINSTACK, NULL);
  switch (lua_type(L, -1)) {
  case LUA_TTABLE:
    mark_table(L, dL, parent, desc);
    break;
  case LUA_TUSERDATA:
    mark_userdata(L, dL, parent, desc);
    break;
  case LUA_TFUNCTION:
    mark_function(L, dL, parent, desc);
    break;
  case LUA_TTHREAD:
    mark_thread(L, dL, parent, desc);
    break;
  default:
    lua_pop(L, 1);
    break;
  }
}

static int luac_snapshot(lua_State* L) {
  lua_gc(L, LUA_GCCOLLECT);
  lua_State *dL = luaL_newstate();
  for (int i = 0; i < MARK; i++) {
    lua_newtable(dL);
  }
  lua_pushvalue(L, LUA_REGISTRYINDEX);
  mark_table(L, dL, NULL, "[registry]");
  lua_pushvalue(dL, 1);
  lua_xmove(dL, L, TABLE);
  lua_close(dL);
  return 1;
}

/********************************************************************************/

SKYNET_API int luaopen_profiler(lua_State* L) {
  const luaL_Reg methods[] = {
    //{ "snapshot",   luac_snapshot }, /* snapshot */
    { NULL,         NULL          }
  };
  return new_module(L, LUA_GNAME, methods);
}

/********************************************************************************/
