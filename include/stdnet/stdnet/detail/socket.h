

#ifndef STDNET_BASIC_SOCKET_H
#define STDNET_BASIC_SOCKET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/timer.h"
#include "stdnet/detail/ssl/stream.h"

#define TRUST_TIMEOUT    300000
#define UNTRUST_TIMEOUT  5000

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

ASIO_DECL ip::tcp::endpoint resolve(
  const char* host, unsigned short port, error_code& ec) {
  if (!host || !host[0]) {
    return ip::tcp::endpoint(ip::tcp::v6(), port);
  }
  io_context ios;
  ip::tcp::resolver resolver(ios);
  auto result = resolver.resolve(host, std::to_string(port), ec);
  return result.empty() ? ip::tcp::endpoint() : result.begin()->endpoint();
}

/***********************************************************************************/

namespace detail {
  typedef service::type_ref io_executor;
  typedef ip::tcp::socket ip_tcp_socket;
  typedef ssl::stream<ip_tcp_socket&> next_layer_type;

  class native_socket : public ip_tcp_socket
    , public std::enable_shared_from_this<native_socket> {
    const identifier _id;

  public:
    typedef std::shared_ptr<
      native_socket
    > type_ref;

  private:
    io_executor   _ios;
    steady::timer _timer;
    next_layer_type* _ssl;
    size_t _lastrcv = 0;
    size_t _timeout = 0;
    const void* _userdata = nullptr;

    using handshake_type = next_layer_type::handshake_type;
    handshake_type _handshake_type;

    struct task_node {
      std::function<
        void(const error_code&, size_t)
      > handler;
      const_buffer buffer;
    };
    bool _closed = false;
    std::list<task_node> send_queue;

    template <typename Handler>
    void on_receive(size_t size, Handler&& handler) {
      error_code ec;
      handler(ec, size);
    }

    void set_timer(size_t expires) {
      auto self(shared_from_this());
      _timer.expires_after(std::chrono::milliseconds(expires));
      _timer.async_wait(
        [expires, this, self](const error_code& ec) {
          if (!ec) {
            on_timer(expires);
          }
        }
      );
    }

    void clear() {
      auto iter = send_queue.begin();
      for (; iter != send_queue.end(); ++iter) {
        free((char*)iter->buffer.data());
      }
      send_queue.clear();
    }

    void flush_next(const error_code& ec, size_t total) {
      if (ec) {
        auto iter = send_queue.begin();
        if (iter != send_queue.end()) {
          iter->handler(ec, 0);
        }
        return;
      }
      while (total > 0) {
        if (send_queue.empty()) {
          break;
        }
        auto iter = send_queue.begin();
        auto size = iter->buffer.size();
        total -= size;

        iter->handler(ec, size);
        free((char*)iter->buffer.data());
        send_queue.pop_front();
      }
      flush_next(); //flush continue...
    }

    void flush_next() {
      if (_closed || !is_open()) {
        clear();
        return;
      }
      if (send_queue.empty()) {
        if (_closed) {
          error_code ec;
          ip_tcp_socket::close(ec);
        }
        return;
      }
      std::vector<const_buffer> buffers;
      auto iter = send_queue.begin();
      for (; iter != send_queue.end(); ++iter) {
        buffers.push_back(iter->buffer);
      }
      auto self(shared_from_this());
      async_write(*this, buffers,
        [this, self](const error_code& ec, size_t total) {
          flush_next(ec, total); //flush continue...
        }
      );
    }

    template<typename ConstBufferSequence, typename Handler>
    void wait_send(const ConstBufferSequence& buffers, Handler&& handler) {
      if (!buffers.data()) {
        handler(error::no_memory, 0);
        return;
      }
      task_node node;
      node.buffer = buffers;
      node.handler = handler;
      send_queue.push_back(node);
      if (send_queue.size() == 1) {
        flush_next(); //not sending...
      }
    }

  protected:
    virtual void on_timer(size_t expires) {
      size_t now = steady_clock();
      if (now - _lastrcv >= _timeout) {
        close();
        return;
      }
      set_timer(_timeout / 5);
    }

  public:
    ASIO_DECL static type_ref create() {
      return std::make_shared<native_socket>();
    }

    ASIO_DECL static type_ref create(ssl::context& ctx) {
      return std::make_shared<native_socket>(ctx);
    }

    ASIO_DECL static type_ref create(io_executor ios) {
      return std::make_shared<native_socket>(ios);
    }

    ASIO_DECL static type_ref create(io_executor ios, ssl::context& ctx) {
      return std::make_shared<native_socket>(ios, ctx);
    }

    ASIO_DECL ~native_socket() {
      clear();
      if (_ssl) {
        delete _ssl;
      }
    }

    ASIO_DECL void set_timeout(size_t timeout) {
      if (_timeout == 0) {
        set_timer(timeout / 5);
        _lastrcv = steady_clock();
      }
      _timeout = timeout;
    }

    ASIO_DECL int id() const {
      return _id.value();
    }

    ASIO_DECL native_socket()
      : native_socket(service::local()) {
    }

    ASIO_DECL native_socket(ssl::context& ctx)
      : native_socket(service::local(), ctx) {
    }

    ASIO_DECL native_socket(io_executor ios)
      : ip_tcp_socket(*ios)
      , _timer(ios)
      , _ios(ios)
      , _ssl(nullptr)
      , _handshake_type(handshake_type::server) {
    }

    ASIO_DECL native_socket(io_executor ios, ssl::context& ctx)
      : ip_tcp_socket(*ios)
      , _timer(ios)
      , _ios(ios)
      , _ssl(new next_layer_type(*this, ctx))
      , _handshake_type(handshake_type::server) {
    }

  public:
    ASIO_DECL io_executor get_executor() const {
      return _ios;
    }
    
    ASIO_DECL void async_close() {
      auto self(shared_from_this());
      get_executor()->post([this, self]() {
        close();
      });
    }

    virtual void close() {
      if (_closed) {
        return;
      }
      error_code ec;
      if (is_idle()) {
        ip_tcp_socket::close(ec);
      }
      _timer.cancel();
      _closed = true;
    }

    virtual bool is_native() const {
      return _ssl == nullptr;
    }

    ASIO_DECL bool is_client() const {
      return _handshake_type == handshake_type::client;
    }

    ASIO_DECL bool is_idle() const {
      return send_queue.empty();
    }

    ASIO_DECL bool is_security() const {
      return _ssl != nullptr;
    }

    ASIO_DECL void userdata(const void* ud) {
      _userdata = ud;
    }

    ASIO_DECL const void* userdata() const {
      return _userdata;
    }

  public:
    template<typename MutableBufferSequence>
    ASIO_DECL size_t receive(
      const MutableBufferSequence& buffers, error_code& ec) {
      return read_some(buffers, ec);
    }

    template<typename MutableBufferSequence>
    ASIO_DECL size_t read_some(
      const MutableBufferSequence& buffers, error_code& ec) {
      return _ssl ? _ssl->read_some(buffers, ec) : ip_tcp_socket::read_some(buffers, ec);
    }

    ASIO_DECL size_t send(
      const char* data, size_t size, error_code& ec) {
      return send(ASIO_BUFFER(data, size), ec);
    }

    template<typename ConstBufferSequence>
    ASIO_DECL size_t send(
      const ConstBufferSequence& buffers, error_code& ec) {
      return write_some(buffers, ec);
    }

    template <typename ConstBufferSequence>
    ASIO_DECL size_t write_some(
      const ConstBufferSequence& buffers, error_code& ec) {
      return _ssl ? _ssl->write_some(buffers, ec) : ip_tcp_socket::write_some(buffers, ec);
    }

    ASIO_SYNC_OP_VOID handshake(error_code& ec) {
      _ssl ? _ssl->handshake(_handshake_type, ec) : ec.clear();
    }

    ASIO_SYNC_OP_VOID connect(const endpoint_type& peer, error_code& ec) {
      _handshake_type = handshake_type::client;
      ip_tcp_socket::connect(peer, ec);
    }

    ASIO_SYNC_OP_VOID connect(const char* host, unsigned short port, error_code& ec) {
      auto peer = resolve(host, port, ec);
      if (!ec) {
        connect(peer, ec);
      }
    }

  public:
    template<typename MutableBufferSequence, typename ReadHandler>
    void async_receive(
      const MutableBufferSequence& buffers, ReadHandler&& handler) {
      auto self(shared_from_this());
      async_read_some(buffers,
        [handler, this, self](const error_code& ec, size_t size) {
          ec ? handler(ec, 0) : on_receive(size, handler);
        }
      );
    }

    template<typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(
      const MutableBufferSequence& buffers, ReadHandler&& handler) {
      _lastrcv = steady_clock();
      if (_timeout == 0) {
        if (is_native()) {
          set_timeout(TRUST_TIMEOUT);
        } else {
          set_timeout(UNTRUST_TIMEOUT);
        }
      }
      _ssl ? _ssl->async_read_some(buffers, handler) : ip_tcp_socket::async_read_some(buffers, handler);
    }

    template<typename WriteHandler>
    void async_send(
      const char* data, size_t size, WriteHandler&& handler) {
      async_send(ASIO_BUFFER(data, size), handler);
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_send(
      const ConstBufferSequence& buffers, WriteHandler&& handler) {
      auto size = buffers.size();
      auto buff = ASIO_BUFFER(malloc(size), size);
      if (buff.data()) {
        memcpy(buff.data(), buffers.data(), size);
      }
      auto self(shared_from_this());
      get_executor()->post([handler, buff, this, self]() {
        wait_send(buff, handler);
      });
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(
      const ConstBufferSequence& buffers, WriteHandler&& handler) {
      _ssl ? _ssl->async_write_some(buffers, handler) : ip_tcp_socket::async_write_some(buffers, handler);
    }

    template <typename Handler>
    void async_handshake(Handler&& handler) {
      auto self(shared_from_this());
      if (!_ssl) {
        get_executor()->post([handler, this, self]() {
          error_code ec;
          handler(ec);
        });
        return;
      }
      _ssl->async_handshake(_handshake_type,
        [handler, this, self](const error_code& ec) {
          handler(ec);
        }
      );
    }

    template <typename Handler>
    void async_connect(const endpoint_type& peer, Handler&& handler) {
      _handshake_type = handshake_type::client;
      ip_tcp_socket::async_connect(peer, handler);
    }

    template <typename Handler>
    void async_connect(const char* host, unsigned short port, Handler&& handler) {
      error_code ec;
      auto peer = resolve(host, port, ec);
      if (!ec) {
        async_connect(peer, handler);
        return;
      }
      get_executor()->post([handler, ec]() { handler(ec); });
    }
  }; //end of class
} //end of namespace detail

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_BASIC_SOCKET_H
