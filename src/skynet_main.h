

#ifndef SKYNET_PMAIN_H
#define SKYNET_PMAIN_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "skynet_lua.h"

/***********************************************************************************/

SKYNET_API void skynet_main(lua_State* L, int argc, const char* argv[]);

/***********************************************************************************/

#endif //SKYNET_PMAIN_H
