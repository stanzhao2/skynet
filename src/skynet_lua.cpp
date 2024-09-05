

#include "skynet_lua.h"
#include "skynet_allotor.h"

/* open lua libs */
#include "core/lua_core.h"
#include "extend/lua_compile.h"
#include "extend/lua_json.h"
#include "extend/lua_list.h"
#include "extend/lua_deflate.h"
#include "extend/lua_http.h"
#include "extend/lua_openssl.h"
#include "extend/lua_string.h"
#include "extend/lua_storage.h"
#include "extend/lua_directory.h"

/********************************************************************************/

static const lua_CFunction skynet_modules[] = {
  luaopen_core,         /* os.wait ...    */
  luaopen_compile,      /* os.compile     */
  luaopen_json,         /* json.encode... */
  luaopen_list,         /* list */
  luaopen_http,         /*  */
  luaopen_deflate,      /* deflate, inflate */
  luaopen_base64,       /* base64.encode  */
  luaopen_crypto,       /* crypto.sha1 ... */
  luaopen_lstring,      /* string.split    */
  luaopen_storage,      /*  */
  luaopen_directory,    /* os.mkdir, os.opendir */
  NULL
};

/********************************************************************************/

static int  panic(lua_State* L);
static void warnfoff (void *ud, const char *message, int tocont);
static void warnfon  (void *ud, const char *message, int tocont);
static void warnfcont(void *ud, const char *message, int tocont);

/********************************************************************************/

static void openlibs(lua_State* L, const lua_CFunction f[]) {
  lua_gc(L, LUA_GCSTOP);
  luaL_openlibs(L);
  while (f && *f) {
    lua_pushcfunction(L, *f++);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
      lua_error(L);
    }
  }
  lua_gc(L, LUA_GCRESTART);
  lua_gc(L, LUA_GCGEN, 0, 0);
}

static lua_State* newstate() {
  lua_State* L = lua_newstate(skynet_allotor, NULL);
  if (L) {
    lua_atpanic (L, &panic);
    lua_setwarnf(L, warnfoff, L); /* default is warnings off */
    openlibs(L, skynet_modules);
  }
  return L;
}

SKYNET_API lua_State* skynet_local() {
  static thread_local lua_State* L = newstate();
  return L;
}

SKYNET_API lua_State* skynet_newstate() {
  return newstate();
}

SKYNET_API typeof<io::service> skynet_service() {
  return io::service::local();
}

/********************************************************************************/

static int panic(lua_State *L) {
  const char *msg = lua_tostring(L, -1);
  if (msg == NULL) {
    msg = "error object is not a string";
  }
  lua_writestringerror("PANIC: unprotected error in call to Lua API (%s)\n", msg);
  return 0;
}

static int checkcontrol(lua_State *L, const char *message, int tocont) {
  if (tocont || *(message++) != '@')  /* not a control message? */
    return 0;
  else {
    if (strcmp(message, "off") == 0)
      lua_setwarnf(L, warnfoff, L);  /* turn warnings off */
    else if (strcmp(message, "on") == 0)
      lua_setwarnf(L, warnfon, L);   /* turn warnings on */
    return 1;  /* it was a control message */
  }
}

static void warnfcont(void *ud, const char *message, int tocont) {
  lua_State *L = (lua_State *)ud;
  lua_writestringerror("%s", message);  /* write message */
  if (tocont) { /* not the last part? */
    lua_setwarnf(L, warnfcont, L);  /* to be continued */
  }
  else {  /* last part */
    lua_writestringerror("%s", "\n");  /* finish message with end-of-line */
    lua_setwarnf(L, warnfon, L);  /* next call is a new message */
  }
}

static void warnfon(void *ud, const char *message, int tocont) {
  if (checkcontrol((lua_State*)ud, message, tocont)) { /* control message? */
    return;  /* nothing else to be done */
  }
  lua_writestringerror("%s", "Lua warning: ");  /* start a new warning */
  warnfcont(ud, message, tocont);  /* finish processing */
}

static void warnfoff(void *ud, const char *message, int tocont) {
  checkcontrol((lua_State *)ud, message, tocont);
}

/********************************************************************************/
