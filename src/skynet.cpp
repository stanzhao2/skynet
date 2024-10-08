﻿

#include "skynet.h"
#include "skynet_main.h"

/***********************************************************************************/

static void copyright(const char* filename) {
#ifdef _MSC_VER
  const char* platform = "windows";
#else
  const char* platform = "linux";
#endif
  printf("%s for %s release v%s\n", filename, platform, SKYNET_VERSION);
  printf("...%s\n", LUA_RELEASE);
#ifdef STDNET_USE_DEFLATE
  printf("...zlib %s\n", ZLIB_VERSION);
#endif
#ifdef STDNET_USE_OPENSSL
  printf("...%s\n", OPENSSL_VERSION_TEXT);
#endif
  printf("\n");
  printf("Usage: %s lua-module [...]\n", filename);
  printf("\n");
}

static const char* parse_progname(const char* filename) {
  const char* prog = strrchr(filename, *PSDIR);
  return prog ? prog + 1 : filename;
}

/***********************************************************************************/

SKYNET_API bool is_debugging() {
#if defined(LUA_DEBUG) || defined(_MSC_VER)
  return true;
#else
  return false;
#endif
}

SKYNET_API int main(int argc, const char* argv[]) {
#ifdef _MSC_VER
  SetConsoleOutputCP(65001);
#endif
  const char* progname = argv[0];
  progname = parse_progname(progname);
  if (argc <= 1) {
    copyright(progname); //copyright
  }
  else {
    lua_State* L = lua_local();
    luaL_checkversion(L);
    skynet_main(L, argc - 1, argv + 1);
    lua_close(L);
    printf("%s has exits normally.\n\n", progname);
  }
  return 0;
}

/***********************************************************************************/
