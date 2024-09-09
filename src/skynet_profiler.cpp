

#include "skynet_lua.h"
#include "skynet_profiler.h"

struct luanode {
  std::string type;
  std::string file;
  size_t size;
  size_t line;
  size_t resize_count;
};

static thread_local lua_Debug ar;
static thread_local std::map<void*, luanode> memorys;

/********************************************************************************/

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
