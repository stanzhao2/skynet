

/***********************************************************************************/

#ifndef __CONFIGURE_WINDOWS_H_
#define __CONFIGURE_WINDOWS_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <direct.h>

#include "stdnet/os/generic.h"
#include "stdnet/os/struct/semaphore.h"
#include "stdnet/os/struct/circular_buffer.h"
#include "stdnet/os/co/coroutine.h"

/***********************************************************************************/

#ifndef PSDIR
#define PSDIR "\\"
#endif

#ifndef LIBEXT
#define LIBEXT "dll"
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif

#define getcwd    _getcwd
#define chdir     _chdir
#define mkdir     _mkdir
#define rmdir     _rmdir
#define getpid    _getpid
#define stricmp   _stricmp

/***********************************************************************************/
/* link with -ldl */

CONF_API void* dlopen(const char* filename, int flag = 0) {
  (void)flag; /* not used */
  return (void*)LoadLibraryA(filename);
}

CONF_API void* dlsym(void* handle, const char* symbol) {
  return GetProcAddress((HMODULE)handle, symbol);
}

CONF_API char* dlerror() {
  static thread_local char info[1024];
  info[0] = '\0';
  FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    info,
    sizeof(info),
    NULL
  );
  return info;
}

CONF_API void dlclose(void* handle) {
  FreeLibrary((HMODULE)handle);
}

/***********************************************************************************/

CONF_API const char* getpwd(char* path, size_t size) {
  DWORD rslt = GetModuleFileNameA(NULL, path, (DWORD)size);
  if (rslt == 0) {
    return nullptr;
  }
  path[rslt] = '\0';
  for (int i = (int)rslt; i >= 0; i--) {
    if (path[i] == *PSDIR) {
      path[i ? i : 1] = '\0';
      break;
    }
  }
  return path;
}

CONF_API bool make_dir(const char* path) {
  HANDLE hfind;
  WIN32_FIND_DATAA data;
  hfind = FindFirstFileA(path, &data);

  if (hfind != INVALID_HANDLE_VALUE) {
    FindClose(hfind);
    return true;
  }
  return (_mkdir(path) == 0);
}

/***********************************************************************************/

#endif //__CONFIGURE_WINDOWS_H_

/***********************************************************************************/
