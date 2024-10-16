

#include "../skynet.h"
#include "lua_print.h"

/********************************************************************************/

static const char* tostring(lua_State* L) {
  size_t n = 0;
  int argc = lua_gettop(L);
  std::string str;
  for (int i = 1; i <= argc; i++) {
    auto p = luaL_tolstring(L, i, &n);
    str.append(p, n);
    lua_pop(L, 1);
    if (i < argc) {
      str.append("\t", 1);
    }
  }
  lua_pushlstring(L, str.c_str(), str.size());
  return luaL_checkstring(L, -1);
}

static lua_State *getthread(lua_State *L) {
  return lua_isthread(L, 1) ? lua_tothread(L, 1) : L;
}

static const char* fixstr(const char* in, size_t inlen, char* out, size_t outlen) {
  if (inlen < outlen) {
    return in;
  }
  size_t first = outlen / 3;
  size_t second = outlen - first - 4;
  strncpy(out, in, first);
  strncpy(out + first, "...", 4);
  strncpy(out + first + 3, in + inlen - second, second + 1);
  return out;
}

static const char* fileline(lua_State* L) {
  L = getthread(L);
  luaL_Buffer buf;
  luaL_buffinit(L, &buf);
  char filename[1024] = { 0 };
  for (int i = 0; i < 10; i++) {
    lua_Debug ar;
    if (!lua_getstack(L, i, &ar)) {
      break;
    }
    if (!lua_getinfo(L, "Sl", &ar)) {
      break;
    }
    if (ar.currentline > 0) {
      if (*ar.source == '@') {
        ar.source++;
        ar.srclen--;
      }
      char temp[512];
      auto str = fixstr(ar.source, ar.srclen, temp, sizeof(temp));
      snprintf(filename, sizeof(filename), "<%s:%d>", str, ar.currentline);
      luaL_addstring(&buf, filename);
      break;
    }
  }
  luaL_pushresult(&buf);
  return luaL_checkstring(L, -1);
}

static const char* localtime(char* out, size_t size) {
  time_t now = time(NULL);
  struct tm* ptm = localtime(&now);
  snprintf(out, size, "[%02d/%02d %02d:%02d:%02d] ", ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return out;
}

static void foutput(const char* msg, color_type color) {
  char prefix[128];
  static std::mutex printlock;
  std::unique_lock<std::mutex> lock(printlock);
  localtime(prefix, sizeof(prefix));

#ifndef _MSC_VER
  fprintf(stdout, "\033[1;36m%s\033[0m", prefix);
  switch (color) {
  case color_type::print:
    fprintf(stdout, "\033[1;36m%s\033[0m", msg);
    break;
  case color_type::trace:
    fprintf(stdout, "\033[1;33m%s\033[0m", msg);
    break;
  case color_type::error:
    fprintf(stdout, "\033[1;31m%s\033[0m", msg);
    break;
  default: break;
  }
#else
  CONSOLE_SCREEN_BUFFER_INFO si;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(h, &si);
  WORD text_color = FOREGROUND_INTENSITY;
  switch (color) {
  case color_type::print:
    text_color |= FOREGROUND_GREEN;
    text_color |= FOREGROUND_BLUE;
    break;
  case color_type::trace:
    text_color |= FOREGROUND_RED;
    text_color |= FOREGROUND_GREEN;
    break;
  case color_type::error:
    text_color |= FOREGROUND_RED;
    break;
  default: break;
  }
  WORD defclr = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE;
  SetConsoleTextAttribute(h, defclr);
  fprintf(stdout, "%s", prefix);
  SetConsoleTextAttribute(h, text_color);
  fprintf(stdout, "%s", msg);
  SetConsoleTextAttribute(h, si.wAttributes);
#endif
}

static int luac_throw(lua_State* L) {
  luaL_error(L, "%s", tostring(L));
  return 0;
}

static int luac_print(lua_State* L) {
  const char* msg = tostring(L);
  const char* pos = fileline(L);
  lua_printf(color_type::print, "%s %s\n", pos, msg);
  return 0;
}

static int luac_trace(lua_State* L) {
  if (is_debugging(L)) {
    const char* msg = tostring(L);
    const char* pos = fileline(L);
    lua_printf(color_type::trace, "%s %s\n", pos, msg);
  }
  return 0;
}

static int luac_error(lua_State* L) {
  const char* msg = tostring(L);
  const char* pos = fileline(L);
  lua_printf(color_type::error, "%s %s\n", pos, msg);
  return 0;
}

/********************************************************************************/

int luaopen_print(lua_State* L) {
  const luaL_Reg methods[] = {
    { "print",    luac_print    }, /* print (...) */
    { "trace",    luac_trace    }, /* trace (...) */
    { "error",    luac_error    }, /* error (...) */
    { "throw",    luac_throw    }, /* throw (...) */
    { NULL,       NULL          }
  };
  return new_module(L, LUA_GNAME, methods);
}

SKYNET_API void lua_printf(color_type color, const char* fmt, ...) {
  char cache[4096];
  va_list va_list_args;
  va_start(va_list_args, fmt);
  size_t bytes = vsnprintf(0, 0, fmt, va_list_args);

  char* message = cache;
  if (bytes + 1 > sizeof(cache)) {
    message = (char*)::malloc(bytes + 1);
    if (!message) {
      return;
    }
  }
  va_start(va_list_args, fmt);
  vsprintf(message, fmt, va_list_args);
  va_end(va_list_args);
  foutput(message, color);
  if (message != cache) ::free(message);
}

/********************************************************************************/
