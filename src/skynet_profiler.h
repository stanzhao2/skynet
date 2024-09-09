

#ifndef SKYNET_PROFILER_H
#define SKYNET_PROFILER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "skynet_lua.h"

/********************************************************************************/

SKYNET_API int luaopen_profiler(lua_State* L);

/********************************************************************************/

#endif //SKYNET_PROFILER_H
