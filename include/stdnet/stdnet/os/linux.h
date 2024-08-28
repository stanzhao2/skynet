

/***********************************************************************************/

#ifndef __CONFIGURE_LINUX_H_
#define __CONFIGURE_LINUX_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <linux/kernel.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>    //dlopen, dlclose, dlsym
#include <dirent.h>

#include "stdnet/os/generic.h"
#include "stdnet/os/struct/semaphore.h"
#include "stdnet/os/struct/circular_buffer.h"
#include "stdnet/os/co/coroutine.h"

/***********************************************************************************/

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#ifndef PSDIR
#define PSDIR "/"
#endif

#ifndef LIBEXT
#define LIBEXT "so"
#endif

#define stricmp strcasecmp

/***********************************************************************************/

CONF_API const char* getpwd(char* path, size_t size) {
  int i, rslt = readlink("/proc/self/exe", path, size - 1);
  if (rslt < 0) {
    return nullptr;
  }
  path[rslt] = '\0';
  for (i = rslt; i >= 0; i--) {
    if (path[i] == *PSDIR) {
      path[i ? i : 1] = '\0';
      break;
    }
  }
  return path;
}

CONF_API bool make_dir(const char* path) {
  DIR* dir = opendir(path);
  if (dir != 0) {
    closedir(dir);
    return true;
  }
  mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
  return (mkdir(path, mode) == 0);
}

/***********************************************************************************/

#endif //__CONFIGURE_LINUX_H_

/***********************************************************************************/
