

#ifndef SKYNET_LUA_H
#define SKYNET_LUA_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "core/lua_core.h"
#include "core/lua_print.h"  /* print, trace, trace   */
#include "core/lua_pcall.h"  /* lua_pcall, lua_xpcall */
#include "core/lua_wrap.h"   /* lua_wrap, lua_unwrap  */
#include "core/lua_dofile.h" /* skynet_service(id)    */

/***********************************************************************************/

SKYNET_API bool is_debugging(lua_State* L);
SKYNET_API lua_State* skynet_state();
SKYNET_API typeof<io::service> skynet_service();

#define lua_local()   skynet_state()
#define lua_service() skynet_service()

/***********************************************************************************/

#endif //SKYNET_LUA_H
