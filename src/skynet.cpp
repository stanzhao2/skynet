

#include "skynet.h"
#include "skynet_main.h"

/***********************************************************************************/

static std::atomic_bool stopped  = false;
static std::atomic_bool debuging = false;

struct cmd_handler {
  const char* name;
  bool (*handler)(int, const char*[]);
};

static bool cmd_debug(int argc, const char* argv[]) {
  if (argc == 1) {
    std::string option(argv[0]);
    if (option == "on") {
      debuging = true;
      printf("debugging has been turned on\n");
      return true;
    }
    if (option == "off") {
      debuging = false;
      printf("debugging has been turned off\n");
      return true;
    }
  }
  return false;
}

static bool cmd_exit(int argc, const char* argv[]) {
  return stopped = true;
}

static bool cmd_intevel(int argc, const char* argv[]) {
  static const cmd_handler handlers[] = {
    { "exit",   cmd_exit    },
    { "debug",  cmd_debug   },
    { NULL,     NULL        }
  };
  auto next = handlers;
  const char* cmd = argv[0];
  while (next->name) {
    if (strcmp(cmd, next->name) == 0) {
      return next->handler(argc - 1, argv + 1);
    }
    next++;
  }
  return false;
}

static void lua_pload(int argc, const char* argv[]) {
  skynet_pmain(skynet_local(), argc, argv);
}

static bool is_delimiter(char c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\0';
}

static void run_command(const char* cmd) {
  int i = 0, j = 0;
  char  part[1024];
  const char* argv[1024];
  for (argv[0] = 0; *cmd; cmd++) {
    char c = *cmd;
    if (is_delimiter(c)) {
      if (j > 0) {
        part[j++] = '\0';
        argv[i] = new char[j];
        memcpy((char*)argv[i++], part, j);
        j = 0;
        argv[i] = 0;
      }
      continue;
    }
    part[j++] = c;
  }
  if (i > 0) {
    if (!cmd_intevel(i, argv)) {
      lua_pload(i, argv);
    }
    for (; i >= 0; i--) {
      if (argv[i]) delete[] argv[i];
    }
  }
}

static void copyright(const char* filename) {
  printf("%s %s\n", filename, __TIMESTAMP__);
  printf("Copyright iccgame.com, All right reserved\n");
  printf("\n");
}

static void on_signal(int code) {
  signal(code, on_signal);
  if (code == SIGTERM) stopped = true;
}

static void print_prompt() {
  char cwd[MAX_PATH];
  signal(SIGINT,  on_signal);
  signal(SIGTERM, on_signal);
  printf("%s> ", getcwd(cwd, sizeof(cwd)));
}

static const char* parse_progname(const char* filename) {
  const char* prog = strrchr(filename, *PSDIR);
  return prog ? prog + 1 : filename;
}

SKYNET_API bool is_debugging() {
#ifdef _DEBUG
  return true;
#else
  return debuging;
#endif
}

SKYNET_API int main(int argc, const char* argv[]) {
#ifdef _MSC_VER
  SetConsoleOutputCP(65001);
#endif
  auto L = skynet_local();
  const char* progname = argv[0];
  progname = parse_progname(progname);
  if (argc > 1) {
    lua_pload(argc - 1, argv + 1);
  }
  else {
    copyright(progname); //copyright
    char cache[8192];
    const char* cmd = nullptr;
    while (!stopped) {
      print_prompt();
      cmd = fgets(cache, sizeof(cache), stdin);
      if (!cmd) {
        printf("\n"); /* Ctrl+C */
        continue;
      }
      if (cmd == (char*)EOF) {
        printf("\n"); /* Killed */
        break;
      }
      while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t') {
          cmd++;
          continue;
        }
        if (*cmd) {
          run_command(cmd);
        }
        break;
      }
    }
  }
  lua_close(L);
  printf("%s has exited.\n\n", progname);
  return 0;
}

/***********************************************************************************/
