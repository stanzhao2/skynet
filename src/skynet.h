

#ifndef SKYNET_H
#define SKYNET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "skynet_lua.h"

#define SKYNET_VERSION "1.0.3"

/***********************************************************************************/

SKYNET_API bool is_debugging();
SKYNET_API int  skynet_execute(int argc, const char* argv[]);

/***********************************************************************************/

#endif //SKYNET_H
