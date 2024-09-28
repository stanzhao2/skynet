

#include "lua_string.h"

/********************************************************************************/

static int luac_isalnum(lua_State* L) {
  const char* p = luaL_checkstring(L, 1);
  while (*p) {
    if (isalnum(*p++) == 0) {
      lua_pushboolean(L, 0);
      return 1;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int luac_isalpha(lua_State* L) {
  const char* p = luaL_checkstring(L, 1);
  while (*p) {
    if (isalpha(*p++) == 0) {
      lua_pushboolean(L, 0);
      return 1;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int luac_trim(lua_State* L) {
  size_t size;
  const char* p = luaL_checklstring(L, 1, &size);
  if (size == 0) {
    return 1;
  }
  const char* p1 = p, *p2 = p + size - 1;
  while (*p1) {
    if (*p1 == ' ') { p1++; continue; }
    if (*p2 == ' ') { p2--; continue; }
    break;
  }
  lua_pushlstring(L, p1, p2 - p1 + 1);
  return 1;
}

static int luac_icmp(lua_State* L) {
  const char* a = luaL_checkstring(L, 1);
  const char* b = luaL_checkstring(L, 2);
  lua_pushboolean(L, stricmp(a, b) == 0 ? 1 : 0);
  return 1;
}

static int luac_split(lua_State* L) {
  const char *s = luaL_checkstring(L, 1);
  const char *sep = luaL_checkstring(L, 2);
  const char *e;
  int i = 1;

  lua_newtable(L);  /* result */
                    /* repeat for each separator */
  while ((e = strchr(s, *sep)) != NULL) {
    lua_pushlstring(L, s, e-s);  /* push substring */
    lua_rawseti(L, -2, i++);
    s = e + 1;  /* skip separator */
  }
  /* push last substring */
  lua_pushstring(L, s);
  lua_rawseti(L, -2, i);
  return 1;  /* return the table */
}

/********************************************************************************/

SKYNET_API int luaopen_lstring(lua_State* L) {
  const luaL_Reg methods[] = {
    { "split",    luac_split    }, /* string.split(s, r) */
    { "trim",     luac_trim     }, /* string.trim(s) */
    { "icmp",     luac_icmp     }, /* string.icmp(a, b) */
    { "isalnum",  luac_isalnum  }, /* string.isalnum(s)  */
    { "isalpha",  luac_isalpha  }, /* string.isalpha(s)  */
    { NULL,       NULL          }
  };
  return new_module(L, "string", methods);
}

/********************************************************************************/
