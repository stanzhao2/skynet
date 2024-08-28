

#ifndef __LUA_PLOAD_H
#define __LUA_PLOAD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "../skynet_lua.h"

/********************************************************************************/

enum struct pload_state {
  pending, exited, error, successfully
};

struct pload_context {
  semaphore     lock;
  lua_State*    L;
  typeof<io::service> ios;
  pload_state   state;
  std::string   name;
  std::string   argv;
  std::string   error;
  std::shared_ptr<std::thread> thread;
};

/********************************************************************************/

SKYNET_API int luaopen_pload(lua_State *L);
SKYNET_API pload_context* local_context(lua_State* L);

/********************************************************************************/

#endif //__LUA_PLOAD_H
