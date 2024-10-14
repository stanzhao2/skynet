

#include "../skynet.h"
#include "lua_rpcall.h"
#include <mutex>
#include <map>
#include <set>
#include <vector>

/********************************************************************************/

typedef struct __node_type {
  size_t who = 0;
  int rcb = 0; /* callback refer */
  int opt = 0; /* enable call by remote */
  inline bool operator<(const __node_type& r) const {
    return who < r.who;
  }
} node_type;

struct pend_invoke {
  int caller;
  int rcf;
  size_t timeout;
};

typedef std::string topic_type;

typedef std::set<
  node_type
> rpcall_set_type;

typedef std::map<
  topic_type, rpcall_set_type
> rpcall_map_type;

typedef std::map<
  size_t, pend_invoke
> invoke_map_type;

#define max_expires  10000
#define is_local(what) (what <= 0xffff)
#define unique_mutex_lock(what) std::unique_lock<std::mutex> lock(what)

static std::mutex _mutex;
static int watcher_ios  = 0;
static int watcher_luaf = 0;
static lua_CFunction   watcher_cfn = nullptr;
static rpcall_map_type rpcall_handlers;

static thread_local size_t rpcall_nextid = 0;
static thread_local size_t rpcall_caller = 0;
static thread_local int    rpcall_r_handler = 0;
static thread_local invoke_map_type invoke_pendings;

/********************************************************************************/

static inline size_t next_sn() {
  return ++rpcall_nextid;
}

static inline int remove_of_pending(size_t sn) {
  auto iter = invoke_pendings.find(sn);
  if (iter != invoke_pendings.end()) {
    int rcf = iter->second.rcf;
    invoke_pendings.erase(iter);
    return rcf;
  }
  return 0;
}

static void insert_of_pending(int caller, int rcf, size_t sn, size_t timeout) {
  pend_invoke pend;
  pend.caller  = caller;
  pend.rcf     = rcf;
  pend.timeout = steady_clock() + timeout;
  invoke_pendings[sn] = pend;
}

static int watch_handler(lua_State* L) {
  if (watcher_cfn != nullptr) {
    return watcher_cfn(L);
  }
  if (watcher_luaf == 0) {
    return 0;
  }
  int argc = lua_gettop(L);
  int type = lua_pushref(L, watcher_luaf);
  if (type != LUA_TFUNCTION) {
    return 0;
  }
  lua_rotate(L, 1, 1);
  if (lua_pcall(L, argc, 0, 0) != LUA_OK) {
    lua_ferror("%s\n", luaL_checkstring(L, -1));
  }
  return 0;
}

static void cancel_invoke(int rcf, size_t sn) {
  rcf = remove_of_pending(sn);
  if (rcf == 0) {
    return;
  }
  auto L = lua_local();
  lua_auto_revert revert(L);
  lua_auto_unref  unref_rcf(L, rcf);

  int typeof_ref = lua_pushref(L, rcf);
  if (typeof_ref == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    int nret = 0;
    lua_pushboolean(coL, 0); /* false */
    lua_pushliteral(coL, "timeout");
    int state = lua_resume(coL, L, 2, &nret);
    if (nret > 0) {
      lua_xmove(coL, L, nret);
      lua_pop(L, nret);
    }
    return;
  }
  if (typeof_ref == LUA_TFUNCTION) {
    lua_pushboolean(L, 0); /* false */
    lua_pushliteral(L, "timeout");
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
      lua_ferror("%s\n", luaL_checkstring(L, -1));
    }
  }
}

static void cancel_block(const std::string& data, int rcf, size_t sn) {
  rcf = remove_of_pending(sn);
  if (rcf == 0) {
    return;
  }
  auto service = find_service(std::abs(rcf));
  if (!service) {
    return;
  }
  std::string* pcontext = (std::string*)service->get_context();
  if (pcontext) {
    pcontext->assign(data);
    service->wakeup();
  }
}

/* calling the caller callback function */
static void back_to_local(const std::string& data, int rcf, size_t sn) {
  rcf = remove_of_pending(sn);
  if (rcf == 0) {
    return;
  }
  lua_State* L = lua_local();
  lua_auto_revert revert(L);
  lua_auto_unref  unref_rcf(L, rcf);

  int typeof_ref = lua_pushref(L, rcf);
  if (typeof_ref == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    lua_pushlstring(coL, data.c_str(), data.size());
    int argc  = lua_unwrap(coL);
    int nret  = 0;
    int state = lua_resume(coL, L, argc, &nret);
    if (nret > 0) {
      lua_xmove(coL, L, nret);
      lua_pop(L, nret);
    }
    return;
  }
  if (typeof_ref == LUA_TFUNCTION) {
    lua_pushlstring(L, data.c_str(), data.size());
    int argc = lua_unwrap(L);
    if (lua_pcall(L, argc, 0, 0) != LUA_OK) {
      lua_ferror("%s\n", luaL_checkstring(L, -1));
    }
  }
}

static int check_timeout(size_t now) {
  auto iter = invoke_pendings.begin();
  for (; iter != invoke_pendings.end(); ++iter) {
    auto timeout = iter->second.timeout;
    if (now < timeout) {
      continue;
    }
    auto rcf = iter->second.rcf;
    if (rcf < 0) {
      continue;
    }
    auto caller = iter->second.caller;
    auto service = find_service(caller);
    if (service) {
      service->post(_bind(cancel_invoke, rcf, iter->first));
    }
  }
  return LUA_OK;
}

static int check_timeout(lua_State* L, size_t expires) {
  static thread_local size_t _lastgc = rand();
  static thread_local steady_timer _timer(*lua_service());
  /* GC every one minute */
  size_t gc_expires = is_debugging() ? expires : 60000;
  if ((_lastgc += expires) >= gc_expires) {
    _lastgc = 0;
    lua_gc(L, LUA_GCCOLLECT);
  }
  /* check timeout for rpcall */
  check_timeout(steady_clock());
  _timer.expires_after(std::chrono::milliseconds(expires));
  _timer.async_wait(
    [L, expires](const error_code& ec) {
      if (!ec) check_timeout(L, expires);
    }
  );
  return LUA_OK;
}

/* back to response of request */
static void forword(const std::string& data, size_t caller, int rcf, size_t sn) {
  auto service = find_service(watcher_ios);
  if (service) {
    service->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_createtable(L, 0, 5);
        lua_pushliteral(L, evr_response);
        lua_setfield(L, -2, "what");
        lua_pushlstring(L, data.c_str(), data.size());
        lua_setfield(L, -2, "data");
        lua_pushinteger(L, (lua_Integer)caller);
        lua_setfield(L, -2, "caller");
        lua_pushinteger(L, (lua_Integer)rcf);
        lua_setfield(L, -2, "rcf");
        lua_pushinteger(L, (lua_Integer)sn);
        lua_setfield(L, -2, "sn");
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
          lua_ferror("%s\n", lua_tostring(L, -1));
        }
      }
    );
  }
}

/* send deliver request */
/*
** who: receiver
** rcf: callback function reference for the caller
*/
static int forword(const topic_type& topic, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf, size_t sn) {
  std::string argv;
  if (data && size) {
    argv.assign(data, size);
  }
  auto service = find_service(watcher_ios);
  if (service) {
    service->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_createtable(L, 0, 8);
        lua_pushliteral(L, evr_deliver);
        lua_setfield(L, -2, "what");
        lua_pushlstring(L, topic.c_str(), topic.size());
        lua_setfield(L, -2, "name");
        lua_pushlstring(L, argv.c_str(), argv.size());
        lua_setfield(L, -2, "argv");
        lua_pushinteger(L, (lua_Integer)mask);
        lua_setfield(L, -2, "mask");
        lua_pushinteger(L, (lua_Integer)who);
        lua_setfield(L, -2, "who");
        lua_pushinteger(L, (lua_Integer)caller);
        lua_setfield(L, -2, "caller");
        lua_pushinteger(L, (lua_Integer)rcf);
        lua_setfield(L, -2, "rcf");
        lua_pushinteger(L, (lua_Integer)sn);
        lua_setfield(L, -2, "sn");
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
          lua_ferror("%s\n", lua_tostring(L, -1));
        }
      }
    );
  }
  return service ? 1 : 0;
}

/* send bind or unbind request */
static int dispatch(const topic_type& topic, const std::string& what, int rcb, size_t caller) {
  auto service = find_service(watcher_ios);
  if (service) {
    service->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_createtable(L, 0, 4);
        lua_pushlstring(L, what.c_str(), what.size());
        lua_setfield(L, -2, "what");
        lua_pushlstring(L, topic.c_str(), topic.size());
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, (lua_Integer)rcb);
        lua_setfield(L, -2, "rcb");
        lua_pushinteger(L, (lua_Integer)caller);
        lua_setfield(L, -2, "caller");
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
          lua_ferror("%s\n", lua_tostring(L, -1));
        }
      }
    );
  }
  return service ? 1 : 0;
}

static int response(const char* data, size_t size, size_t caller, int rcf, size_t sn) {
  /* if don't need result */
  if (rcf == 0) {
    return 0;
  }
  std::string temp(data, size);
  if (is_local(caller)) {
    lua_r_response(temp, caller, rcf, sn);
  }
  else {
    forword(temp, caller, rcf, sn);
  }
  return 1;
}

static int responser(lua_State* L) {
  lua_pushboolean(L, 1);
  lua_insert(L, 1);
  lua_wrap(L, lua_gettop(L));
  size_t size = 0;
  const char* data = luaL_checklstring(L, -1, &size);
  size_t caller    = luaL_checkinteger(L, lua_upvalueindex(1));
  int rcf          = (int)luaL_checkinteger(L, lua_upvalueindex(2));
  size_t sn        = luaL_checkinteger(L, lua_upvalueindex(3));
  response(data, size, caller, rcf, sn);
  return 0;
}

/* calling the receiver function */
/*
** who: receiver
** rcf: callback function reference for the caller
*/
static int dispatch(const topic_type& topic, int rcb, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf, size_t sn) {
  /* who is receiver */
  if (!is_local(who)) {
    return forword(topic, data, size, mask, who, caller, rcf, sn);
  }
  std::string argv;
  if (data && size) {
    argv.assign(data, size);
  }
  auto service = find_service((int)who);
  if (!service) {
    return 0;
  }
  service->post(
    [=]() {
      lua_State* L = lua_local();
      lua_auto_revert revert(L);
      int type = lua_pushref(L, rcb);
      if (type != LUA_TFUNCTION) {
        lua_pushboolean(L, 0);
        lua_pushliteral(L, "function not found");
        lua_wrap(L, 2);
        size_t nsize;
        const char* result = luaL_checklstring(L, -1, &nsize);
        response(result, nsize, caller, rcf, sn);
        return;
      }
      int argc = 0;
      if (!argv.empty()) {
        lua_pushlstring(L, argv.c_str(), argv.size());
        argc += lua_unwrap(L);
      }
      int prev_handler = rpcall_r_handler;
      if (rcf != 0) {
        lua_pushinteger(L, caller);
        lua_pushinteger(L, rcf);
        lua_pushinteger(L, sn);
        lua_pushcclosure(L, responser, 3);
        rpcall_r_handler = lua_ref(L, -1);
        lua_pop(L, 1);
      }
      size_t previous = rpcall_caller;
      rpcall_caller = caller;
      int callok = lua_pcall(L, argc, LUA_MULTRET, 0);
      rpcall_caller = previous;

      bool need_response = false;
      if (callok != LUA_OK) {
        lua_ferror("%s\n", lua_tostring(L, -1));
      }
      if (rpcall_r_handler > 0) {
        need_response = true;
        lua_unref(L, rpcall_r_handler);
      }
      rpcall_r_handler = prev_handler;
      /* call success and need't response */
      if (callok == LUA_OK && !need_response) {
        /* maybe will run in coroutine */
        return;
      }
      lua_pushboolean(L, callok == LUA_OK ? 1 : 0);
      int count = lua_gettop(L) - revert.top();
      if (count > 1) {
        lua_rotate(L, -count, 1);
      }
      lua_wrap(L, count);
      size_t nsize;
      const char* result = luaL_checklstring(L, -1, &nsize);
      response(result, nsize, caller, rcf, sn);
    }
  );
  return 1;
}

/********************************************************************************/

static int luac_r_caller(lua_State* L) {
  lua_pushinteger(L, (lua_Integer)rpcall_caller);
  return 1;
}

static int luac_r_handler(lua_State* L) {
  if (rpcall_r_handler == 0) {
    lua_pushnil(L);
  }
  else {
    lua_pushref(L, rpcall_r_handler);
    rpcall_r_handler = 0;
  }
  return 1;
}

/* ignoring return values */
static int luac_deliver(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t mask = luaL_checkinteger(L, 2);
  size_t who  = luaL_checkinteger(L, 3);

  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 3;
  if (argc > 0) {
    lua_wrap(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  int caller = lua_service()->id();
  int count  = lua_r_deliver(name, data, size, mask, who, caller, 0, next_sn());
  lua_pushinteger(L, count);
  return 1;
}

/* async wait return values */
static int luac_invoke(lua_State* L) {
  const char* name = luaL_checkstring (L, 1);
  size_t mask      = luaL_checkinteger(L, 2);
  size_t receiver  = luaL_checkinteger(L, 3);
  size_t timeout   = luaL_checkinteger(L, 4);
  if (mask == 0 && receiver == 0) {
    mask = rand() + 1;
  }
  int rcf = lua_ref(L, 5); /* callback */
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 5;
  if (argc > 0) {
    lua_wrap(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto sn = next_sn();
  auto service = lua_service();
  auto caller = service->id();
  int count = lua_r_deliver(name, data, size, mask, receiver, caller, rcf, sn);
  if (count == 0) {
    std::string fname(name);
    service->post([rcf, fname]() {
      lua_State* L = lua_local();
      lua_auto_revert revert(L);
      lua_auto_unref  unref(L, rcf);
      lua_pushref(L, rcf);
      lua_pushboolean(L, 0);
      lua_pushfstring(L, "%s not found", fname.c_str());
      if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        lua_ferror("%s\n", lua_tostring(L, -1));
      }
    });
  }
  else {
    insert_of_pending(caller, rcf, sn, timeout);
  }
  lua_pushboolean(L, count > 0 ? 1 : 0);
  return 1;
}

static int luac_rpcall(lua_State* L) {
  /* async call */
  if (lua_type(L, 5) == LUA_TFUNCTION) {
    return luac_invoke(L);
  }
  const char* name = luaL_checkstring (L, 1);
  size_t mask      = luaL_checkinteger(L, 2);
  size_t receiver  = luaL_checkinteger(L, 3);
  size_t timeout   = luaL_checkinteger(L, 4);
  if (mask == 0 && receiver == 0) {
    mask = rand() + 1;
  }
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 4;
  if (argc > 0) {
    lua_wrap(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto sn = next_sn();
  int  caller = lua_service()->id();
  int  rcf = 0 - caller;
  /* if invoke by coroutine */
  if (lua_isyieldable(L)) {
    lua_State* main = lua_local();
    lua_pushthread(L);
    lua_xmove(L, main, 1);
    rcf = lua_ref(main, -1);
    lua_pop(main, 1);
  }
  int count = lua_r_deliver(name, data, size, mask, receiver, caller, rcf, sn);
  if (count == 0) {
    if (rcf > 0) {
      lua_unref(L, rcf);
    }
    lua_pushboolean(L, 0); /* false */
    lua_pushfstring(L, "%s not found", name);
    return 2;
  }
  /* run in coroutine */
  insert_of_pending(caller, rcf, sn, timeout);
  if (rcf > 0) {
    lua_settop(L, 0);
    return lua_yield(L, lua_gettop(L));
  }
  /* call will be blocked */
  std::string rpcall_ret;
  auto service = lua_service();
  service->set_context(&rpcall_ret);
  bool result = service->wait_for(timeout);
  remove_of_pending(sn);
  service->set_context(nullptr);

  if (service->stopped()) {
    lua_pushboolean(L, 0); /* false */
    lua_pushliteral(L, "cancel");
    return 2;
  }
  if (!result) {
    lua_pushboolean(L, 0); /* false */
    lua_pushliteral(L, "timeout");
    return 2;
  }
  lua_pushlstring(L, rpcall_ret.c_str(), rpcall_ret.size());
  return lua_unwrap(L);
}

/* register function */
static int luac_declare(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int opt = luaL_optboolean(L, 3, 0);
  int who = lua_service()->id();
  int rcb = lua_ref(L, 2);
  int result = lua_r_bind(name, who, rcb, opt);
  if (result != LUA_OK) {
    lua_unref(L, rcb);
    lua_pushboolean(L, 0);
  }
  else {
    lua_pushboolean(L, 1);
    if (opt) dispatch(name, evr_bind, rcb, who);
  }
  return 1;
}

/* unregister function */
static int luac_undeclare(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  int who = lua_service()->id();
  int opt = 0;
  int rcb = lua_r_unbind(name, who, &opt);
  if (rcb) {
    lua_unref(L, rcb);
    if (opt) dispatch(name, evr_unbind, rcb, who);
  }
  lua_pushboolean(L, rcb ? 1 : 0);
  return 1;
}

/* watch events */
static int luac_lookout(lua_State* L) {
  if (lua_isnoneornil(L, 1)) {
    if (watcher_luaf) {
      lua_unref(L, watcher_luaf);
      watcher_ios  = 0;
      watcher_luaf = 0;
    }
    return 0;
  }
  luaL_checktype(L, 1, LUA_TFUNCTION);
  if (watcher_luaf) {
    luaL_error(L, "lookout can only be called once");
  }
  watcher_ios  = lua_service()->id();
  watcher_luaf = lua_ref(L, 1);
  return 0;
}

static int luac_r_bind(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t who = luaL_checkinteger(L, 2);
  int rcb = (int)luaL_checkinteger(L, 3);
  int result = lua_r_bind(name, who, rcb, 0);
  lua_pushboolean(L, result == LUA_OK ? 1 : 0);
  return 1;
}

static int luac_r_unbind(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  size_t who = luaL_checkinteger(L, 2);
  int rcb = lua_r_unbind(name, who, nullptr);
  lua_pushboolean(L, rcb > 0 ? 1 : 0);
  return 1;
}

static int luac_r_response(lua_State* L) {
  size_t size;
  const char* data = luaL_checklstring(L, 1, &size);
  size_t caller = luaL_checkinteger(L, 2);
  if (!is_local(caller)) {
    luaL_error(L, "#2 out of range: %d", caller);
  }
  int rcf = (int)luaL_checkinteger(L, 3);
  size_t sn = luaL_checkinteger(L, 4);
  std::string argv(data, size);
  int result = lua_r_response(argv, caller, rcf, sn);
  lua_pushboolean(L, result == LUA_OK ? 1 : 0);
  return 1;
}

static int luac_r_deliver(lua_State* L) {
  size_t size;
  const char* name = luaL_checkstring(L, 1);
  const char* data = luaL_checklstring(L, 2, &size);
  size_t mask      = luaL_checkinteger(L, 3);
  size_t who       = luaL_checkinteger(L, 4);
  size_t caller    = luaL_checkinteger(L, 5);
  if (!is_local(who)) {
    luaL_error(L, "#4 out of range: %d", who);
  }
  if (is_local(caller)) {
    luaL_error(L, "#5 out of range: %d", caller);
  }
  int rcf = (int)luaL_checkinteger(L, 6);
  size_t sn = luaL_checkinteger(L, 7);
  int count = lua_r_deliver(name, data, size, mask, who, caller, rcf, sn);
  lua_pushinteger(L, count);
  return 1;
}

/********************************************************************************/

struct lua_newrpc final {
  inline static const char* name() {
    return "skynet rpcall";
  }
  inline static lua_newrpc* __this(
    lua_State* L, int index = 1) {
    return checkudata<lua_newrpc>(L, index, name());
  }
  static int __gc(lua_State* L) {
    auto self = __this(L);
#ifdef LUA_DEBUG
    lua_ftrace("DEBUG: %s will gc\n", name());
#endif
    self->~lua_newrpc();
    return 0;
  }
  static int __call(lua_State* L) {
    auto self = __this(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    lua_pushinteger(L, (lua_Integer)self->mask);
    lua_pushinteger(L, (lua_Integer)self->receiver);
    lua_pushinteger(L, (lua_Integer)self->timeout);
    lua_rotate(L, 3, 3);
    lua_remove(L, 1); /* remove self */
    return luac_rpcall(L);
  }
  static int dispatch(lua_State* L) {
    auto self = __this(L);
    luaL_checktype(L, 2, LUA_TSTRING);
    lua_pushinteger(L, (lua_Integer)self->mask);
    lua_pushinteger(L, (lua_Integer)self->receiver);
    lua_rotate(L, 3, 2);
    lua_remove(L, 1); /* remove self */
    return luac_deliver(L);
  }
  static int set_mask(lua_State* L) {
    auto self = __this(L);
    auto old  = self->mask;
    self->receiver = 0;
    self->mask = luaL_checkinteger(L, 2);
    lua_pushinteger(L, (lua_Integer)old);
    return 1;
  }
  static int set_receiver(lua_State* L) {
    auto self = __this(L);
    auto old  = self->receiver;
    self->mask = 0;
    self->receiver = luaL_checkinteger(L, 2);
    lua_pushinteger(L, (lua_Integer)old);
    return 1;
  }
  static int set_timeout(lua_State* L) {
    auto self = __this(L);
    auto old  = self->timeout;
    self->timeout = luaL_checkinteger(L, 2);
    if (self->timeout < 1000) {
      self->timeout = 1000;
    }
    lua_pushinteger(L, (lua_Integer)old);
    return 1;
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "dispatch", dispatch      },
      { "__call",   __call        },
      { "__gc",     __gc          },
      { "mask",     set_mask      },
      { "timeout",  set_timeout   },
      { "receiver", set_receiver  },
      { NULL,       NULL          }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int create(lua_State* L) {
    auto mask = luaL_optinteger(L, 1, 0);
    auto receiver = luaL_optinteger(L, 2, 0);
    if (mask && receiver) {
      luaL_error(L, "#1 and # 2 can't both be greater than 0");
    }
    auto timeout = luaL_optinteger(L, 3, max_expires);
    if (timeout < 1000) {
      timeout = 1000;
    }
    auto self = newuserdata<lua_newrpc>(L, name());
    self->mask     = mask;
    self->receiver = receiver;
    self->timeout  = timeout;
    return 1;
  }
  size_t mask, receiver, timeout;
};

static int luac_create(lua_State* L) {
  return lua_newrpc::create(L);
}

/********************************************************************************/

SKYNET_API int luaopen_rpcall(lua_State* L) {
  lua_newrpc::init_metatable(L);
  const luaL_Reg methods[] = {
    { "lookout",    luac_lookout    },
    { "create",     luac_declare    },
    { "new",        luac_create     },
    { "remove",     luac_undeclare  },
    { "caller",     luac_r_caller   },
    { "responser",  luac_r_handler  },

    { "r_deliver",  luac_r_deliver  },
    { "r_bind",     luac_r_bind     },
    { "r_unbind",   luac_r_unbind   },
    { "r_response", luac_r_response },
    { NULL,         NULL            }
  };
  new_module(L, "rpc", methods);
  return check_timeout(L, 1000);
}

SKYNET_API int lua_l_lookout(lua_CFunction f) {
  unique_mutex_lock(_mutex);
  watcher_ios = lua_service()->id();
  watcher_cfn = f;
  return LUA_OK;
}

SKYNET_API int lua_r_unbind(const char* name, size_t who, int* opt) {
  node_type node;
  node.who = who;

  topic_type topic(name);
  unique_mutex_lock(_mutex);
  auto iter = rpcall_handlers.find(topic);
  if (iter == rpcall_handlers.end()) {
    return 0;
  }
  rpcall_set_type& val = iter->second;
  auto find = val.find(node);
  if (find == val.end()) {
    return 0;
  }
  if (opt) {
    *opt = find->opt;
  }
  int rcb = find->rcb;
  val.erase(find);
  if (val.empty()) {
    rpcall_handlers.erase(iter);
  }
  return rcb;
}

SKYNET_API int lua_r_bind(const char* name, size_t who, int rcb, int opt) {
  node_type node;
  node.who = who;
  node.rcb = rcb;
  node.opt = opt;

  topic_type topic(name);
  unique_mutex_lock(_mutex);
  auto iter = rpcall_handlers.find(topic);

  if (iter == rpcall_handlers.end()) {
    rpcall_set_type val;
    val.insert(node);
    rpcall_handlers[topic] = val;
    return LUA_OK;
  }
  rpcall_set_type& val = iter->second;
  auto find = val.find(node);
  if (find == val.end()) {
    val.insert(node);
    return LUA_OK;
  }
  return LUA_ERRRUN;
}

SKYNET_API int lua_r_response(const std::string& data, size_t caller, int rcf, size_t sn) {
  if (is_local(caller)) {
    if (rcf < 0) {
      auto service = find_service(std::abs(rcf));
      if (service) {
        service->post(_bind(cancel_block, data, rcf, sn));
        return LUA_OK;
      }
    }
    else {
      auto service = find_service((int)caller);
      if (service) {
        service->post(_bind(back_to_local, data, rcf, sn));
        return LUA_OK;
      }
    }
  }
  return LUA_ERRRUN;
}

SKYNET_API int lua_r_deliver(const char* name, const char* data, size_t size, size_t mask, size_t who, size_t caller, int rcf, size_t sn) {
  node_type node;
  node.who = who;
  topic_type topic(name);

  unique_mutex_lock(_mutex);
  auto iter = rpcall_handlers.find(topic);
  if (iter == rpcall_handlers.end()) {
    return 0;
  }
  rpcall_set_type& val = iter->second;
  if (val.empty()) {
    return 0;
  }
  if (!is_local(caller) && !is_local(who)) {
    return 0;
  }
  size_t receiver = who;
  /* if the mask is set */
  if (mask > 0) {
    std::vector<node_type> select;
    for (auto find = val.begin(); find != val.end(); ++find) {
      /* can't call by remote */
      if (find->opt == 0 && !is_local(caller)) {
        continue;
      }
      if (is_local(caller) || is_local(who)) {
        select.push_back(*find);
      }
    }
    if (!select.empty()) {
      auto i = mask % select.size();
      node.who = receiver = select[i].who;
    }
  }
  /* if there is a receiver */
  if (receiver > 0) {
    auto find = val.find(node);
    if (find == val.end()) {
      return 0;
    }
    if (!is_local(caller)) {
      /* can't call by remote */
      if (find->opt == 0) {
        return 0;
      }
    }
    auto rcb = find->rcb;
    return dispatch(topic, rcb, data, size, mask, receiver, caller, rcf, sn);
  }
  /* dispatch to all receivers */
  int count = 0;
  auto find = val.begin();
  for (; find != val.end(); ++find) {
    auto rcb = find->rcb;
    auto who = find->who;
    count += dispatch(topic, rcb, data, size, mask, who, caller, rcf, sn);
  }
  return count;
}

/********************************************************************************/
