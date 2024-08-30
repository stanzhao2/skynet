

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
static thread_local invoke_map_type invoke_pendings;

/********************************************************************************/

static inline size_t next_sn() {
  return ++rpcall_nextid;
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
  if (lua_pcall(L, argc, 0) != LUA_OK) {
    lua_ferror("%s\n", luaL_checkstring(L, -1));
  }
  return 0;
}

static void cancel_invoke(int rcf) {
  auto L = lua_local();
  lua_auto_revert revert(L);
  lua_auto_unref  unref_rcf(L, rcf);

  int type = lua_pushref(L, rcf);
  if (type == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    int argc = 1;
    lua_pushboolean(coL, 0); /* false */
    lua_pushstring(coL, "timeout");
    lua_resume(coL, L, 2, &argc);
    return;
  }
  if (type == LUA_TFUNCTION) {
    lua_pushboolean(L, 0); /* false */
    if (lua_pcall(L, 1, 0) != LUA_OK) {
      lua_ferror("%s\n", luaL_checkstring(L, -1));
    }
  }
}

static int check_timeout() {
  static thread_local steady_timer _timer(*lua_service());
  auto now  = steady_clock();
  auto iter = invoke_pendings.begin();
  for (; iter != invoke_pendings.end();) {
    auto timeout = iter->second.timeout;
    if (now < timeout) {
      ++iter;
      continue;
    }
    auto rcf = iter->second.rcf;
    auto caller = iter->second.caller;
    iter = invoke_pendings.erase(iter);

    auto executor = find_service(caller);
    if (executor) {
      executor->post(_bind(cancel_invoke, rcf));
    }
  }
  _timer.expires_after(std::chrono::milliseconds(1000));
  _timer.async_wait(
    [](const error_code& ec) {
      if (!ec) check_timeout();
    }
  );
  return LUA_OK;
}

/* calling the caller callback function */
static void back_to_local(const std::string& data, int rcf, size_t sn) {
  lua_State* L = lua_local();
  lua_auto_revert revert(L);
  lua_auto_unref  unref_rcf(L, rcf);
  auto iter = invoke_pendings.find(sn);
  if (iter == invoke_pendings.end()) {
    return;
  }
  invoke_pendings.erase(iter);
  int type = lua_pushref(L, rcf);
  if (type == LUA_TTHREAD) {
    auto coL = lua_tothread(L, -1);
    if (lua_status(coL) != LUA_YIELD) {
      return;
    }
    lua_pushlstring(coL, data.c_str(), data.size());
    int argc = lua_unwrap(coL);
    lua_resume(coL, L, argc, &argc);
    return;
  }
  if (type == LUA_TFUNCTION) {
    lua_pushlstring(L, data.c_str(), data.size());
    int argc = lua_unwrap(L);
    if (lua_pcall(L, argc, 0) != LUA_OK) {
      lua_ferror("%s\n", luaL_checkstring(L, -1));
    }
  }
}

/* back to response of request */
static void forword(const std::string& data, size_t caller, int rcf, size_t sn) {
  auto executor = find_service(watcher_ios);
  if (executor) {
    executor->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_newtable(L);
        lua_pushstring(L, evr_response);
        lua_setfield(L, -2, "what");
        lua_pushlstring(L, data.c_str(), data.size());
        lua_setfield(L, -2, "data");
        lua_pushinteger(L, (lua_Integer)caller);
        lua_setfield(L, -2, "caller");
        lua_pushinteger(L, (lua_Integer)rcf);
        lua_setfield(L, -2, "rcf");
        lua_pushinteger(L, (lua_Integer)sn);
        lua_setfield(L, -2, "sn");
        if (lua_pcall(L, 1, 0) != LUA_OK) {
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
  auto executor = find_service(watcher_ios);
  if (executor) {
    executor->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_newtable(L);
        lua_pushstring(L, evr_deliver);
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
        if (lua_pcall(L, 1, 0) != LUA_OK) {
          lua_ferror("%s\n", lua_tostring(L, -1));
        }
      }
    );
  }
  return executor ? 1 : 0;
}

/* send bind or unbind request */
static int dispatch(const topic_type& topic, const std::string& what, int rcb, size_t caller) {
  auto executor = find_service(watcher_ios);
  if (executor) {
    executor->post(
      [=]() {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_pushcfunction(L, watch_handler);
        lua_newtable(L);
        lua_pushlstring(L, what.c_str(), what.size());
        lua_setfield(L, -2, "what");
        lua_pushlstring(L, topic.c_str(), topic.size());
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, (lua_Integer)rcb);
        lua_setfield(L, -2, "rcb");
        lua_pushinteger(L, (lua_Integer)caller);
        lua_setfield(L, -2, "caller");
        if (lua_pcall(L, 1, 0) != LUA_OK) {
          lua_ferror("%s\n", lua_tostring(L, -1));
        }
      }
    );
  }
  return executor ? 1 : 0;
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
  auto executor = find_service((int)who);
  if (!executor) {
    return 0;
  }
  executor->post(
    [=]() {
      lua_State* L = lua_local();
      lua_auto_revert revert(L);
      int type = lua_pushref(L, rcb);
      if (type != LUA_TFUNCTION) {
        return;
      }
      int argc = 0;
      if (!argv.empty()) {
        lua_pushlstring(L, argv.c_str(), argv.size());
        argc += lua_unwrap(L);
      }
      size_t previous = rpcall_caller;
      rpcall_caller = caller;

      int callok = lua_pcall(L, argc, LUA_MULTRET);
      if (callok != LUA_OK) {
        lua_ferror("%s\n", lua_tostring(L, -1));
      }
      rpcall_caller = previous;
      /* don't need result */
      if (rcf == 0) {
        return;
      }
      lua_pushboolean(L, callok == LUA_OK ? 1 : 0);
      int count = lua_gettop(L) - revert.top();
      if (count > 1) {
        lua_rotate(L, -count, 1);
      }
      /* return result */
      lua_wrap(L, count);
      size_t nsize;
      const char* result = luaL_checklstring(L, -1, &nsize);
      std::string temp(result, nsize);
      if (is_local(caller)) {
        lua_r_response(temp, caller, rcf, sn);
        return;
      }
      forword(temp, caller, rcf, sn);
    }
  );
  return 1;
}

/********************************************************************************/

static int luac_caller(lua_State* L) {
  lua_pushinteger(L, (lua_Integer)rpcall_caller);
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
  luaL_checktype(L, 1, LUA_TFUNCTION);
  int rcf = lua_ref(L, 1);
  const char* name = luaL_checkstring(L, 2);
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 2;
  if (argc > 0) {
    lua_wrap(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto sn = next_sn();
  auto caller = lua_service()->id();
  int count = lua_r_deliver(name, data, size, rand(), 0, caller, rcf, sn);
  if (count > 0) {
    pend_invoke pend;
    pend.caller  = caller;
    pend.rcf     = rcf;
    pend.timeout = steady_clock() + max_expires;
    invoke_pendings[sn] = pend;
  }
  else {
    lua_unref(L, rcf);
  }
  lua_pushboolean(L, count > 0 ? 1 : 0);
  return 1;
}

static int luac_rpcall(lua_State* L) {
  if (lua_type(L, 1) == LUA_TFUNCTION) {
    return luac_invoke(L);
  }
  auto executor = lua_service();
  int rcf = 0 - executor->id();
  if (lua_isyieldable(L)) {
    /* in coroutine */
    lua_State* main = lua_local();
    lua_pushthread(L);
    lua_xmove(L, main, 1);
    rcf = lua_ref(main, -1);
    lua_pop(main, 1);
  }
  const char* name = luaL_checkstring(L, 1);
  size_t size = 0;
  const char* data = nullptr;
  int argc = lua_gettop(L) - 1;
  if (argc > 0) {
    lua_wrap(L, argc);
    data = luaL_checklstring(L, -1, &size);
  }
  auto sn = next_sn();
  int caller = executor->id();
  int count  = lua_r_deliver(name, data, size, rand(), 0, caller, rcf, sn);
  if (count == 0) {
    if (rcf > 0) {
      lua_unref(L, rcf);
    }
    lua_pushboolean(L, 0); /* false */
    lua_pushfstring(L, "%s not found", name);
    return 2;
  }
  pend_invoke pend;
  pend.caller  = caller;
  pend.rcf     = rcf;
  pend.timeout = steady_clock() + max_expires;
  invoke_pendings[sn] = pend;
  /* in coroutine */
  if (rcf > 0) {
    return lua_yieldk(L, 0, 0, 0);
  }
  /* not in coroutine */
  std::string rpcall_ret;
  executor->set_context(&rpcall_ret);
  bool result = executor->wait_for(max_expires);
  executor->set_context(nullptr);

  invoke_pendings.erase(sn);
  if (executor->stopped()) {
    lua_pushboolean(L, 0); /* false */
    lua_pushstring(L, "cancel");
    return 2;
  }
  if (!result) {
    lua_pushboolean(L, 0); /* false */
    lua_pushstring(L, "timeout");
    return 2;
  }
  lua_pushlstring(L, rpcall_ret.c_str(), rpcall_ret.size());
  return lua_unwrap(L);
}

/* register function */
static int luac_declare(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int opt = 0;
  if (lua_type(L, 3) == LUA_TBOOLEAN) {
    opt = lua_toboolean(L, 3);
  }
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

SKYNET_API int luaopen_rpcall(lua_State* L) {
  const luaL_Reg methods[] = {
    { "lookout",    luac_lookout    },
    { "declare",    luac_declare    },
    { "undeclare",  luac_undeclare  },
    { "caller",     luac_caller     },
    { "rpcall",     luac_rpcall     },
    { "deliver",    luac_deliver    },

    { "r_deliver",  luac_r_deliver  },
    { "r_bind",     luac_r_bind     },
    { "r_unbind",   luac_r_unbind   },
    { "r_response", luac_r_response },
    { NULL,         NULL            }
  };
  lua_getglobal(L, "os");
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1); /* pop 'os' from stack */

  if (!watcher_ios) {
    watcher_ios = lua_service()->id();
  }
  check_timeout();
  return 0;
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

static void cancel_block(const std::string& data, int rcf, size_t sn) {
  auto iter = invoke_pendings.find(sn);
  if (iter == invoke_pendings.end()) {
    return;
  }
  invoke_pendings.erase(sn);
  auto executor = find_service(std::abs(rcf));
  if (!executor) {
    return;
  }
  std::string* pcontext = (std::string*)executor->get_context();
  if (pcontext) {
    pcontext->assign(data);
    executor->cancel();
  }
}

SKYNET_API int lua_r_response(const std::string& data, size_t caller, int rcf, size_t sn) {
  if (is_local(caller)) {
    if (rcf < 0) {
      auto executor = find_service(std::abs(rcf));
      if (executor) {
        executor->post(_bind(cancel_block, data, rcf, sn));
        return LUA_OK;
      }
    }
    else {
      auto executor = find_service((int)caller);
      if (executor) {
        executor->post(_bind(back_to_local, data, rcf, sn));
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
  /* if the mask is set */
  if (mask) {
    std::vector<node_type> select;
    auto find = val.begin();
    for (; find != val.end(); ++find) {
      /* can't call by remote */
      if (find->opt == 0) {
        if (!is_local(caller)) {
          continue;
        }
      }
      if (is_local(caller) || is_local(who)) {
        select.push_back(*find);
      }
    }
    if (!select.empty()) {
      auto i = mask % select.size();
      who = select[i].who;
      node.who = who;
    }
  }
  /* if there is a receiver */
  if (who) {
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
    return dispatch(topic, rcb, data, size, mask, who, caller, rcf, sn);
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
