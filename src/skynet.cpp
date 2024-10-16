

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

SKYNET_API int skynet_execute(int argc, const char* argv[]) {
  lua_State* L = lua_local();
  luaL_checkversion(L);
  int result = skynet_main(L, argc, argv);
  lua_close(L);
  return result;
}

SKYNET_API int main(int argc, const char* argv[]) {
#ifdef _MSC_VER
  const UINT pagecode = 65001;
  const UINT output_code = GetConsoleOutputCP();
  if (output_code != pagecode) {
    SetConsoleOutputCP(pagecode);
  }
#else
  setenv("LC_CTYPE", "en_US.UTF-8", 1);
#endif
  int result = 0;
  const char* progname = parse_progname(argv[0]);
  if (argc <= 1) {
    copyright(progname); //copyright
  }
  else {
    result = skynet_execute(argc - 1, argv + 1);
    printf("%s has exits normally.\n\n", progname);
  }
  return result;
}

/***********************************************************************************/
