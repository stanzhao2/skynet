﻿

#include "skynet.h"
#include "skynet_main.h"

/***********************************************************************************/

static void copyright(const char* filename) {
  printf("%s %s\n", filename, __TIMESTAMP__);
  printf("Copyright iccgame.com, All right reserved\n");
  printf("\n");
}

static const char* parse_progname(const char* filename) {
  const char* prog = strrchr(filename, *PSDIR);
  return prog ? prog + 1 : filename;
}

/***********************************************************************************/

SKYNET_API bool is_debugging() {
#if defined(STDNET_DEBUG) || defined(_MSC_VER)
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
    skynet_main(L, argc - 1, argv + 1);
    lua_close(L);
    printf("%s has exited.\n\n", progname);
  }
  return 0;
}

/***********************************************************************************/
