

#ifndef STDNET_SSL_STREAM_H
#define STDNET_SSL_STREAM_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/ssl/context.h"

#if !defined(STDNET_USE_OPENSSL)

namespace ssl {
  template <typename SocketType>
  class stream {
    SocketType _ref;
    typedef int verify_mode;

  public:
    enum struct handshake_type {
      client,
      server
    };

  public:
    ASIO_DECL SocketType next_layer() { return _ref; }
    ASIO_DECL void shutdown(error_code&) {}
    ASIO_DECL void set_verify_mode(verify_mode v) {}
    ASIO_SYNC_OP_VOID set_verify_mode(verify_mode v, error_code& ec) { ec = error_if_nossl; }
    ASIO_DECL void set_verify_depth(int depth) {}
    ASIO_SYNC_OP_VOID set_verify_depth(int depth, error_code& ec) { ec = error_if_nossl; }
    template <typename VerifyCallback>
    ASIO_DECL void set_verify_callback(VerifyCallback callback) {}
    template <typename VerifyCallback>
    ASIO_SYNC_OP_VOID set_verify_callback(VerifyCallback callback, error_code& ec) { ec = error_if_nossl; }
    template <typename MutableBufferSequence>
    size_t read_some(const MutableBufferSequence& buffers) { return 0; }
    template <typename MutableBufferSequence>
    size_t read_some(const MutableBufferSequence& buffers, error_code& ec) { ec = error_if_nossl; return 0; }
    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& buffers) { return 0; }
    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& buffers, error_code& ec) { ec = error_if_nossl; return 0; }
    template <typename ConstBufferSequence, typename Handler>
    void async_write_some(const ConstBufferSequence& buffers, Handler&& handler) { }
    template <typename MutableBufferSequence, typename Handler>
    void async_read_some(const MutableBufferSequence& buffers, Handler&& handler) {}
    void handshake(handshake_type type) {}
    ASIO_SYNC_OP_VOID handshake(handshake_type type, error_code& ec) { ec = error_if_nossl; }
    template <typename ConstBufferSequence>
    void handshake(handshake_type type, const ConstBufferSequence& buffers) { }
    template <typename ConstBufferSequence>
    ASIO_SYNC_OP_VOID handshake(handshake_type type, const ConstBufferSequence& buffers, error_code& ec) { ec = error_if_nossl; }

  public:
    template <typename Arg>
    ASIO_DECL stream(Arg& ios, ssl::context& ctx) : _ref(ios) { }

    template <typename Handler>
    void async_handshake(handshake_type type, Handler&& handler) {
      post(next_layer().get_executor(), [handler]() {
        handler(error_if_nossl);
      });
    }
  };
}

#endif //STDNET_USE_OPENSSL

#endif //STDNET_SSL_STREAM_H
