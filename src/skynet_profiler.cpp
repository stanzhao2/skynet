

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
      char key[32];
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

/********************************************************************************/

struct luanode {
  std::string type;
  std::string file;
  size_t size;
  size_t line;
  size_t resize_count;
};

static thread_local lua_Debug ar;
static thread_local std::map<void*, luanode> memorys;

static lua_State* getthread(lua_State *L) {
  return lua_isthread(L, 1) ? lua_tothread(L, 1) : L;
}

static const char* fileline(lua_State* L, size_t& line) {
  L = getthread(L);
  for (int i = 0; i < 10; i++) {
    if (lua_getstack(L, i, &ar)) {
      if (lua_getinfo(L, "Sl", &ar)) {
        if (ar.currentline > 0) {
          line = ar.currentline;
          return ar.short_src;
        }
      }
    }
  }
  return nullptr;
}

static void mem_new(lua_State* L, void* ptr, size_t size, size_t type) {
  lua_auto_revert revert(L);
  luanode node;
  const char* file = fileline(L, node.line);
  if (file) {
    node.file = file;
    node.size = size;
    node.resize_count = 0;
    node.type = lua_typename(L, (int)type);
    memorys[ptr] = node;
  }
}

static void mem_resize(void* ptr, void* nptr, size_t osize, size_t nsize) {
  if (ptr != nptr) {
    auto iter = memorys.find(ptr);
    if (iter != memorys.end()) {
      iter->second.resize_count++;
      iter->second.size = nsize;
      memorys[nptr] = iter->second;
      memorys.erase(iter);
    }
  }
}

void skynet_profiler(void* ptr, void* nptr, size_t osize, size_t nsize) {
  if (nsize == 0) { /* free memory */
    memorys.erase(ptr);
    return;
  }
  if (ptr == nullptr) {
    lua_State* L = lua_local();
    if (L) {
      switch (osize) {
      case LUA_TTABLE:
        mem_new(L, nptr, nsize, osize);
      }
    }
    return;
  }
  mem_resize(ptr, nptr, osize, nsize);
}

/********************************************************************************/
