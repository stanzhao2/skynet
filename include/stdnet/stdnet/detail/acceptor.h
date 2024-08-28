

#ifndef STDNET_ACCEPTOR_H
#define STDNET_ACCEPTOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/websocket.h"

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

namespace detail {
  typedef ip::tcp::acceptor ip_tcp_acceptor;

  class acceptor : public ip_tcp_acceptor {
    const identifier _id;
    io_executor _ios;

  public:
    typedef std::shared_ptr<
      acceptor
    > type_ref;

  private:
    void bind(const endpoint_type& local, error_code& ec) {
      if (!is_open()) {
        ip_tcp_acceptor::open(local.protocol(), ec);
      }
      if (ec) {
        return;
      }
#ifndef _MSC_VER
      reuse_address option(true);
      set_option(option, ec);
      if (ec) {
        return;
      }
#endif
      ip_tcp_acceptor::bind(local, ec);
    }

  public:
    ASIO_DECL acceptor(io_executor ios)
      : ip_tcp_acceptor(*ios)
      , _ios(ios) {
    }

    ASIO_DECL static type_ref create(io_executor ios) {
      return std::make_shared<acceptor>(ios);
    }

    ASIO_DECL int id() const {
      return _id.value();
    }

    ASIO_DECL io_executor get_executor() const {
      return _ios;
    }

    ASIO_DECL void listen(const endpoint_type& local, error_code& ec) {
      bind(local, ec);
      if (!ec) {
        ip_tcp_acceptor::listen(max_listen_connections, ec);
      }
    }

    ASIO_DECL error_code listen(unsigned short port, const char* host = nullptr) {
      error_code ec;
      auto local = resolve(host, port, ec);
      if (!ec) {
        listen(local, ec);
      }
      return ec;
    }

    template <typename Socket>
    ASIO_SYNC_OP_VOID accept(Socket peer, error_code& ec) {
      ip_tcp_acceptor::accept(*peer, ec);
    }

    template <typename Socket, typename Handler>
    ASIO_DECL void async_accept(Socket peer, Handler&& handler) {
      ip_tcp_acceptor::async_accept(*peer, handler);
    }
  }; //end of class
} //end of namespace detail

/***********************************************************************************/

template <typename Acceptor, typename Socket>
void accept(Acceptor acceptor, Socket peer, error_code& ec) {
  acceptor->accept(peer, ec);
  if (!ec) {
    peer->handshake(ec);
  }
}

template <typename Acceptor, typename Socket, typename Accepted, typename Handshaked>
void async_accept(Acceptor acceptor, Socket peer, Accepted&& accepted, Handshaked&& handshaked) {
  acceptor->async_accept(peer, [=](const error_code& ec) {
    /*
    to prevent half-connection attacks,
    the next accept request must be initiated here
    call allotor for async_accept next...
    */
    if (acceptor->is_open()) {
      accepted();
    }
    ec ? handshaked(ec) : peer->async_handshake([=](const error_code& ec) {
      acceptor->get_executor()->post([=]() { handshaked(ec); });
    });
  });
}

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_ACCEPTOR_H
