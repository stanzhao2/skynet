

#ifndef STDNET_LIBRARY_H
#define STDNET_LIBRARY_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#ifndef __cplusplus
#error "stdnet only used for C++ compiler"
#endif

#ifndef STDNET_USE_OPENSSL
/* 
* use -DSTDNET_USE_OPENSSL options enable secure socket
* use link options: -lcrypto -lssl
*/
#endif

#ifndef STDNET_USE_DEFLATE
/* 
* use -DSTDNET_USE_DEFLATE options enable deflate extensions
* use link options: -lz
*/
#endif

#include "stdnet/version.h"
#include "stdnet/detail/timer.h"
#include "stdnet/detail/acceptor.h"
#include "stdnet/detail/websocket.h"

#define _bind std::bind
#define _Arg1 std::placeholders::_1
#define _Arg2 std::placeholders::_2
#define _Arg3 std::placeholders::_3
#define _Arg4 std::placeholders::_4
#define _Arg5 std::placeholders::_5
#define _Arg6 std::placeholders::_6
#define _Arg7 std::placeholders::_7
#define _Arg8 std::placeholders::_8
#define _Arg9 std::placeholders::_9

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

namespace io {
  using service  = detail::service;
  using socket   = detail::socket;
  using acceptor = detail::acceptor;
}

namespace steady {
  using timer = detail::steady::timer;
}

namespace system {
  using timer = detail::system::timer;
}

template<typename Ty>
using typeof = typename Ty::type_ref;

template <typename Ty, typename ...Args>
ASIO_DECL typeof<Ty> ref_new(Args... args) { return Ty::create(args...); }

/***********************************************************************************/

class io_socket_server
  : public std::enable_shared_from_this<io_socket_server> {
  typedef std::function<
    void(const error_code&, typeof<io::socket>)
  > AcceptHander;

  typeof<io::acceptor> _acceptor;
  ssl::context*        _sslctx;
  io::socket::family   _family;
  std::vector<typeof<io::service>> _executors;

  ASIO_DECL typeof<io::service> next_executor() {
    typeof<io::service> next;
    for (auto& ios : _executors) {
      if (!next) {
        next = ios;
        continue;
      }
      if (ios.use_count() < next.use_count()) {
        next = ios;
      }
    }
    return next ? next : _acceptor->get_executor();
  }

  ASIO_DECL void accept_call(
    const error_code& ec, typeof<io::socket> socket, AcceptHander handler) {
    pcall(handler, ec, socket);
  }

  ASIO_DECL void accept_next(AcceptHander handler) {
    auto ios = next_executor();
    typeof<io::socket> socket;
    if (!_sslctx) {
      socket = ref_new<io::socket>(ios, _family);
    }
    else {
      socket = ref_new<io::socket>(ios,
        std::reference_wrapper<ssl::context>(*_sslctx),
        _family
      );
    }
    auto self(shared_from_this());
    async_accept(
      _acceptor, socket,
      _bind(&io_socket_server::accept_next, self, handler),
      _bind(&io_socket_server::accept_call, self, _Arg1, socket, handler)
    );
  }

public:
  typedef std::shared_ptr<
    io_socket_server
  > type_ref;

  ASIO_DECL io_socket_server(
    typeof<io::service> ios, io::socket::family what, ssl::context* pctx)
    : _acceptor(ref_new<io::acceptor>(ios))
    , _family(what)
    , _sslctx(pctx) {
  }

  ASIO_DECL typeof<io::acceptor> native_handle() const {
    return _acceptor;
  }

  ASIO_DECL void add_executor(typeof<io::service> ios) {
    _executors.push_back(ios);
  }

  ASIO_DECL bool is_open() const {
    return _acceptor->is_open();
  }

  ASIO_DECL void close() {
    _acceptor->close();
  }

  ASIO_DECL static type_ref create(
    typeof<io::service> ios, io::socket::family what, ssl::context* pctx) {
    return std::make_shared<io_socket_server>(ios, what, pctx);
  }

  ASIO_DECL static type_ref create(
    typeof<io::service> ios, io::socket::family what) {
    return create(ios, what, nullptr);
  }

  ASIO_DECL static type_ref create(
    typeof<io::service> ios, ssl::context& ctx, io::socket::family what) {
    return create(ios, what, &ctx);
  }

  template <typename Handler>
  ASIO_DECL error_code listen(unsigned short port, const char* host, Handler&& handler) {
    error_code ec = _acceptor->listen(port, host);
    if (!ec) {
      accept_next(handler);
    }
    return ec;
  }
};

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_LIBRARY_H
