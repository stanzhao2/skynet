

#include "lua_socket.h"

/********************************************************************************/

static void push_errcode(lua_State* L, const error_code& ec) {
  auto msg = UTF8(ec.message());
  lua_pushlstring(L, msg.c_str(), msg.size());
}

static void push_endpoint(lua_State* L, const ip::tcp::endpoint& endpoint) {
  lua_newtable(L);
  lua_pushinteger(L, endpoint.port());
  lua_setfield(L, -2, "port");

  auto address = endpoint.address();
  lua_pushstring(L, address.to_string().c_str());
  lua_setfield(L, -2, "address");
}

/********************************************************************************/

struct ssl_context final {
  inline ssl_context()
    : ctx(ssl::context::tlsv12) {
  }
  inline static const char* name() {
    return "ssl context";
  }
  inline static ssl_context* __this(lua_State* L) {
    return checkudata<ssl_context>(L, 1, name());
  }
  static int __gc(lua_State* L) {
    auto self = __this(L);
    self->~ssl_context();
    return 0;
  }
  static int certificate(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    size_t size;
    const char* data = luaL_checklstring(L, 1, &size);
    use_certificate_chain(self->ctx, data, size, ec);
    lua_pushboolean(L, ec ? 0 : 1);
    if (ec) push_errcode(L, ec);
    return ec ? 2 : 1;
  }
  static int key(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    size_t size;
    const char* data = luaL_checklstring(L, 1, &size);
    use_private_key(self->ctx, data, size, ec);
    lua_pushboolean(L, ec ? 0 : 1);
    if (ec) push_errcode(L, ec);
    return ec ? 2 : 1;
  }
  static int password(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    const char* data = luaL_checkstring(L, 1);
    use_private_key_pwd(self->ctx, data, ec);
    lua_pushboolean(L, ec ? 0 : 1);
    if (ec) push_errcode(L, ec);
    return ec ? 2 : 1;
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",         __gc        },
      { "certificate",  certificate },
      { "key",          key         },
      { "password",     password    },
      { NULL,           NULL        },
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int create(lua_State* L) {
    auto self = newuserdata<ssl_context>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushstring(L, "no memory");
      return 2;
    }
    error_code ec;
    init_ssl_context(self->ctx, ec);
    if (ec) {
      lua_pushnil(L);
      push_errcode(L, ec);
    }
    return ec ? 2 : 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    const luaL_Reg methods[] = {
      { "context",  create   }, /* io.context() */
      { NULL,       NULL     }
    };
    lua_getglobal(L, "io");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1); /* pop 'io' from stack */
    return 0;
  }
  ssl::context ctx;
};

/********************************************************************************/

template <typename Ty>
inline typeof<Ty> ref_new_object(lua_State* L) {
  auto ios = io::service::local();
  const std::string what = luaL_checkstring(L, 1);
  if (what == "tcp") {
    return ref_new<Ty>(ios, io::socket::native);
  }
  if (what == "ws") {
    return ref_new<Ty>(ios, io::socket::websocket);
  }
  if (what == "ssl") {
    auto p = checkudata<ssl_context>(L, 2, ssl_context::name());
    return ref_new<Ty>(ios, std::reference_wrapper<ssl::context>(p->ctx), io::socket::native);
  }
  if (what == "wss") {
    auto p = checkudata<ssl_context>(L, 2, ssl_context::name());
    return ref_new<Ty>(ios, std::reference_wrapper<ssl::context>(p->ctx), io::socket::websocket);
  }
  return typeof<Ty>();
}

/********************************************************************************/

struct lua_socket final {
  inline static const char* name() {
    return "lua socket";
  }
  inline static lua_socket* __this(lua_State* L) {
    return checkudata<lua_socket>(L, 1, name());
  }
  static int __gc(lua_State* L) {
    auto self = __this(L);
    self->~lua_socket();
    return 0;
  }
  static int close(lua_State* L) {
    auto self = __this(L);
    self->socket->close();
    return 0;
  }
  static int getid(lua_State* L) {
    auto self = __this(L);
    lua_pushinteger(L, self->socket->id());
    return 1;
  }
  static int set_uri(lua_State* L) {
    auto self = __this(L);
    const char* uri = luaL_checkstring(L, 2);
    self->socket->request_header().uri.assign(uri);
    return 0;
  }
  static int get_uri(lua_State* L) {
    auto self = __this(L);
    auto uri = self->socket->request_header().uri;
    lua_pushlstring(L, uri.c_str(), (lua_Integer)uri.size());
    return 1;
  }
  static int set_header(lua_State* L) {
    auto self = __this(L);
    const char* name  = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    self->socket->request_header().set_header(name, value);
    return 0;
  }
  static int get_header(lua_State* L) {
    auto self = __this(L);
    const char* name = luaL_checkstring(L, 2);
    auto value = self->socket->request_header().get_header(name);
    if (value.empty()) {
      lua_pushnil(L);
    } else {
      lua_pushlstring(L, value.c_str(), (lua_Integer)value.size());
    }
    return 1;
  }
  static int connect(lua_State* L) {
    auto self = __this(L);
    const char* host = luaL_checkstring(L, 2);
    unsigned short port = (unsigned short)luaL_checkinteger(L, 3);
    if (lua_isnone(L, 4)) {
      error_code ec;
      sync_connect(self->socket, host, port, ec);
      lua_pushboolean(L, ec ? 0 : 1);
      if (ec) push_errcode(L, ec);
      return ec ? 2 : 1;
    }
    luaL_checktype(L, 4, LUA_TFUNCTION);
    int handler = lua_ref(L, 4);
    async_connect(self->socket, host, port,
      [handler](const error_code& ec) {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_auto_unref  unref(L, handler);

        lua_pushref(L, handler);
        ec ? push_errcode(L, ec) : lua_pushnil(L);
        if (lua_pcall(L, 1, 0) != LUA_OK) {
          lua_ferror("%s\n", luaL_checkstring(L, -1));
        }
      }
    );
    return 0;
  }
  static int remote_endpoint(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    auto ep = self->socket->remote_endpoint(ec);
    if (ec) {
      lua_pushnil(L);
      push_errcode(L, ec);
      return 2;
    }
    push_endpoint(L, ep);
    return 1;
  }
  static int local_endpoint(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    auto ep = self->socket->local_endpoint(ec);
    if (ec) {
      lua_pushnil(L);
      push_errcode(L, ec);
      return 2;
    }
    push_endpoint(L, ep);
    return 1;
  }
  static int endpoint(lua_State* L) {
    const std::string what = luaL_optstring(L, 2, "remote");
    if (what == "local") {
      return local_endpoint(L);
    }
    if (what == "remote") {
      return remote_endpoint(L);
    }
    lua_pushnil(L);
    lua_pushstring(L, "unknown type");
    return 2;
  }
  static int receive(lua_State* L) {
    auto self = __this(L);
    if (lua_isnone(L, 2)) {
      error_code ec;
      std::string out;
      self->socket->receive(out, ec);
      if (ec) {
        lua_pushnil(L);
        push_errcode(L, ec);
        return 2;
      }
      lua_pushlstring(L, out.c_str(), out.size());
      return 1;
    }
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int handler = lua_ref(L, 2);
    async_receive(self->socket,
      [handler](const error_code& ec, const char* data, size_t size) {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_auto_unref  unref(L, handler);
        if (!ec) {
          unref.cancel();
        }
        lua_pushref(L, handler);
        if (ec) {
          lua_pushnil(L);
        }
        else {
          lua_pushlstring(L, data, size);
        }
        ec ? push_errcode(L, ec) : lua_pushnil(L);
        if (lua_pcall(L, 2, 0) != LUA_OK) {
          lua_ferror("%s\n", luaL_checkstring(L, -1));
        }
      }
    );
    return 0;
  }
  static int send(lua_State* L) {
    auto self = __this(L);
    auto name = lua_typename(L, lua_type(L, 2));
    size_t size = 0;
    const char* data = luaL_checklstring(L, 2, &size);
    if (lua_isnone(L, 3)) {
      error_code ec;
      size = self->socket->send(data, size, ec);
      lua_pushinteger(L, (lua_Integer)size);
      if (ec) {
        push_errcode(L, ec);
        return 2;
      }
      return 1;
    }
    luaL_checktype(L, 3, LUA_TFUNCTION);
    int handler = lua_ref(L, 3);
    self->socket->async_send(data, size,
      [handler](const error_code& ec, size_t size) {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_auto_unref  unref(L, handler);

        lua_pushref(L, handler);
        lua_pushinteger(L, (lua_Integer)size);
        ec ? push_errcode(L, ec) : lua_pushnil(L);
        if (lua_pcall(L, 2, 0) != LUA_OK) {
          lua_ferror("%s\n", luaL_checkstring(L, -1));
        }
      }
    );
    return 0;
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",         __gc        },
      { "connect",      connect     },
      { "id",           getid       },
      { "close",        close       },
      { "endpoint",     endpoint    },
      { "seturi",       set_uri     },
      { "setheader",    set_header  },
      { "geturi",       get_uri     },
      { "getheader",    get_header  },
      { "send",         send        },
      { "receive",      receive     },
      { NULL,           NULL        }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int create(lua_State* L) {
    auto self = newuserdata<lua_socket>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushstring(L, "no memory");
      return 2;
    }
    self->socket = ref_new_object<io::socket>(L);
    if (!self->socket) {
      lua_pushnil(L);
      lua_pushstring(L, "unknown type");
      return 2;
    }
    return 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    const luaL_Reg methods[] = {
      { "socket",  create   }, /* os.timer() */
      { NULL,      NULL     }
    };
    lua_getglobal(L, "io");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1); /* pop '_G' from stack */
    return 0;
  }
  typeof<io::socket> socket;
};

/********************************************************************************/

struct lua_acceptor final {
  inline static const char* name() {
    return "lua server";
  }
  inline static lua_acceptor* __this(lua_State* L) {
    return checkudata<lua_acceptor>(L, 1, name());
  }
  static int __gc(lua_State* L) {
    auto self = __this(L);
    self->~lua_acceptor();
    return 0;
  }
  static int close(lua_State* L) {
    auto self = __this(L);
    self->server->close();
    return 0;
  }
  static int getid(lua_State* L) {
    auto self = __this(L);
    lua_pushinteger(L, self->server->native_handle()->id());
    return 1;
  }
  static int endpoint(lua_State* L) {
    auto self = __this(L);
    error_code ec;
    auto acceptor = self->server->native_handle();
    auto local = acceptor->local_endpoint(ec);
    if (ec) {
      lua_pushnil(L);
      push_errcode(L, ec);
      return 2;
    }
    push_endpoint(L, local);
    return 1;
  }
  static int listen(lua_State* L) {
    auto self = __this(L);
    const char* host = luaL_checkstring(L, 2);
    unsigned short port = (unsigned short)luaL_checkinteger(L, 3);
    int handler = lua_ref(L, 4);
    luaL_checktype(L, 4, LUA_TFUNCTION);

    auto ec = self->server->listen(port, host,
      [handler, self](const error_code& ec, typeof<io::socket> peer) {
        lua_State* L = lua_local();
        lua_auto_revert revert(L);
        lua_auto_unref  unref(L, handler);
        if (!ec) {
          unref.cancel();
        }
        else if (self->server->is_open()) {
          unref.cancel();
        }
        lua_pushref(L, handler);
        auto self = newuserdata<lua_socket>(L, lua_socket::name());
        if (self) {
          self->socket = peer;
        }
        else {
          lua_pushnil(L);
        }
        ec ? push_errcode(L, ec) : lua_pushnil(L);
        if (lua_pcall(L, 2, 0) != LUA_OK) {
          lua_ferror("%s\n", luaL_checkstring(L, -1));
        }
      }
    );
    lua_pushboolean(L, ec ? 0 : 1);
    if (ec) push_errcode(L, ec);
    return ec ? 2 : 1;
  }
  static void init_metatable(lua_State* L) {
    const luaL_Reg methods[] = {
      { "__gc",         __gc        },
      { "listen",       listen      },
      { "id",           getid       },
      { "close",        close       },
      { "endpoint",     endpoint    },
      { NULL,           NULL        }
    };
    newmetatable(L, name(), methods);
    lua_pop(L, 1);
  }
  static int create(lua_State* L) {
    auto self = newuserdata<lua_acceptor>(L, name());
    if (!self) {
      lua_pushnil(L);
      lua_pushstring(L, "no memory");
      return 2;
    }
    self->server = ref_new_object<io_socket_server>(L);
    if (!self->server) {
      lua_pushnil(L);
      lua_pushstring(L, "unknown type");
      return 2;
    }
    return 1;
  }
  static int open_library(lua_State* L) {
    init_metatable(L);
    const luaL_Reg methods[] = {
      { "server",   create   }, /* os.ssl_context() */
      { NULL,       NULL     }
    };
    lua_getglobal(L, "io");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1); /* pop '_G' from stack */
    return 0;
  }
  typeof<io_socket_server> server;
};

/********************************************************************************/

SKYNET_API int luaopen_socket(lua_State* L) {
  int result = 0;
  result += ssl_context::open_library(L);
  result += lua_acceptor::open_library(L);
  result += lua_socket::open_library(L);
  return result;
}

/********************************************************************************/
