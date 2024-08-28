

#ifndef STDNET_WEBSOCKET_H
#define STDNET_WEBSOCKET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/socket.h"
#include "stdnet/detail/wss/handshake.h"
#include "stdnet/detail/wss/decoder.h"

#ifndef STDNET_MAX_PACKET
#define STDNET_MAX_PACKET (8 * 1024 * 1024)
#endif

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

/***********************************************************************************/
namespace detail {
  class socket : public native_socket {
  public:
    enum family {
      websocket, native
    };

    typedef std::shared_ptr<
      socket
    > type_ref;

  private:
    static bool is_utf8(const char* p, size_t n = 0) {
      unsigned char chr;
      size_t i = 0, follow = 0;
      for (; n ? (i < n) : (p[i] != 0); i++) {
        chr = p[i];
        if (follow == 0) {
          if (chr > 0x80) {
            if (chr == 0xfc || chr == 0xfd) {
              follow = 5;
            }
            else if (chr >= 0xf8) {
              follow = 4;
            }
            else if (chr >= 0xf0) {
              follow = 3;
            }
            else if (chr >= 0xe0) {
              follow = 2;
            }
            else if (chr >= 0xc0) {
              follow = 1;
            }
            else {
              return false;
            }
          }
        }
        else {
          follow--;
          if ((chr & 0xC0) != 0x80) {
            return false;
          }
        }
      }
      return (follow == 0);
    }

    ws::opcode_type make_op(const char* s, size_t n) {
      if (_opcode == ws::opcode_type::text) {
        if (!is_utf8(s, n)) {
          _opcode = ws::opcode_type::binary;
        }
      }
      return _opcode;
    }

    std::string& next_packet(std::string& out) {
      out.clear();
      if (!_packets.empty()) {
        out = std::move(_packets.front());
        _packets.pop_front();
      }
      return out;
    }

    error_code decode_of(const char* data, size_t size) {
      error_code ec;
      int result = _decoder.decode(data, size,
        [&](const std::string& data, ws::opcode_type opcode, bool inflate) {
          decode_of(data, opcode, inflate);
        }
      );
      return result ? STDNET_ERROR : ec;
    }

    void push_packet(const std::string& data, bool inflate) {
      if (!inflate) {
        _packets.push_back(data);
        return;
      }
#ifdef STDNET_USE_DEFLATE
      std::string unziped;
      if (zlib::inflate(data, unziped)) {
        _packets.push_back(std::move(unziped));
      }
      else {
        _packets.push_back(data);
      }
#endif
    }

    void on_close(const char* data, size_t size) {
      int code = 0;
      if (size > 2) {
        data = ep_decode16(data, (u16*)&code);
        //printf("Closed[%d]: %s\n", code, data);
      }
      close(code);
    }

    void decode_of(const std::string& data, ws::opcode_type opcode, bool inflate) {
      switch (opcode) {
      case ws::opcode_type::binary:
        _opcode = opcode;

      case ws::opcode_type::text:
        push_packet(data, inflate);
        break;

      case ws::opcode_type::ping:
        pong(data.c_str(), data.size());
        break;

      case ws::opcode_type::close:
        on_close(data.c_str(), data.size());
        break;
      }
    }

    void ping(const char* data, size_t bytes) {
      dispatch(ws::opcode_type::ping, data, bytes);
    }

    void pong(const char* data, size_t bytes) {
      dispatch(ws::opcode_type::pong, data, bytes);
    }

    void close(int code) {
      const char* reason = nullptr;
      switch (code) {
      case 1002:
        reason = "Protocol Error";
        break;
      case 1003:
        reason = "Opcode Unsupported";
        break;
      case 1009:
        reason = "Packet Too Large";
        break;
      default:
        reason = "Normal Closure";
        code = 1000;
        break;
      }
      close(code, reason);
    }

    void close(int code, const char* reason) {
      char data[1024];
      ep_encode16(data, code);
      strcpy(data + 2, reason);
      size_t size = strlen(reason);
      dispatch(ws::opcode_type::close, data, size + 3);
      native_socket::close();
    }

    void dispatch(ws::opcode_type opcode, const char* data, size_t size) {
      if (!_handshaked) {
        return; //handshake not completed
      }
      _encoder.encode(data, size, opcode, false,
        [&](const std::string& data, ws::opcode_type, bool) {
          const char* begin = data.c_str();
          size_t size = data.size();
          if (is_idle()) {
            error_code ec;
            native_socket::send(begin, size, ec);
          }
          else {
            native_socket::async_send(begin, size,
              [](const error_code&, size_t) {}
            );
          }
        }
      );
    }

  private:
    void send_request(error_code& ec) {
      _decoder.set_client();
      _encoder.set_client();

      auto& req = request_header();
      ws::init_request(req);

      auto rdata = req.to_string();
      native_socket::send(rdata.c_str(), rdata.size(), ec);
    }

    void send_response(error_code& ec) {
      auto& res = response_header();
      auto rdata = res.to_string();
      native_socket::send(rdata.c_str(), rdata.size(), ec);

      if (!ec) {
        if (res.status != 101) {
          ec = STDNET_ERROR;
          return;
        }
      }
    }

    void read_request(error_code& ec) {
      auto rbuff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      while (true) {
        auto size = native_socket::receive(rbuff, ec);
        if (ec) {
          return;
        }
        auto& req = request_header();
        const char* begin = _buffer;
        const char* pend = begin + size;
        http::request_parser::result_type result;
        std::tie(result, begin) = _request_parser.parse(req, begin, pend);

        switch (result) {
        case http::response_parser::bad:
          ec = STDNET_ERROR;
        case http::response_parser::good:
          return;
        }
      }
    }

    void read_response(error_code& ec) {
      auto rbuff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      while (true) {
        auto size = native_socket::receive(rbuff, ec);
        if (ec) {
          return;
        }
        auto& res = response_header();
        const char* begin = _buffer;
        const char* pend = begin + size;
        http::response_parser::result_type result;
        std::tie(result, begin) = _response_parser.parse(res, begin, pend);

        switch (result) {
        case http::response_parser::bad:
          ec = STDNET_ERROR;
          return;
        case http::response_parser::good:
          ec = decode_of(begin, pend - begin);
          return;
        }
      }
    }

    void ws_handshake(error_code& ec) {
      auto& req = request_header();
      auto& res = response_header();
      if (is_client()) {
        send_request(ec);
        if (ec) {
          return;
        }
        read_response(ec);
        if (ec) {
          return;
        }
        if (!ws::check_response(res, req)) {
          ec = STDNET_ERROR;
        }
      }
      else {
        read_request(ec);
        ws::init_response(101, res, req);
        send_response(ec);
      }
      if (!ec) {
        _handshaked = true;
        _compress = ws::deflate_enable(res);
      }
    }

    template <typename Handler>
    void read_request(size_t size, Handler&& handler) {
      auto& req = request_header();
      auto& res = response_header();
      const char* begin = _buffer;
      const char* pend = begin + size;
      http::request_parser::result_type result;
      std::tie(result, begin) = _request_parser.parse(req, begin, pend);

      switch (result) {
      case http::request_parser::bad:
        handler(STDNET_ERROR);
        return;
      case http::request_parser::indeterminate:
        read_request(handler);
        return;
      }
      error_code ec;
      ws::init_response(101, res, req);
      send_response(ec);
      if (!ec) {
        set_timeout(TRUST_TIMEOUT);
        _handshaked = true;
        _compress = ws::deflate_enable(res);
      }
      handler(ec);
    }

    template <typename Handler>
    void read_response(size_t size, Handler&& handler) {
      auto& res = response_header();
      auto& req = request_header();
      const char* begin = _buffer;
      const char* pend = begin + size;
      http::response_parser::result_type result;
      std::tie(result, begin) = _response_parser.parse(res, begin, pend);

      switch (result) {
      case http::response_parser::bad:
        handler(STDNET_ERROR);
        return;
      case http::response_parser::indeterminate:
        read_response(handler);
        return;
      }
      if (!ws::check_response(res, req)) {
        handler(STDNET_ERROR);
        return;
      }
      set_timeout(TRUST_TIMEOUT);
      _handshaked = true;
      handler(decode_of(begin, pend - begin));
    }

    template <typename Handler>
    void read_request(Handler&& handler) {
      auto self(shared_from_this());
      auto buff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      native_socket::async_receive(buff,
        [handler, this, self](const error_code& ec, size_t size) {
          ec ? handler(ec) : read_request(size, handler);
        }
      );
    }

    template <typename Handler>
    void read_response(Handler&& handler) {
      auto self(shared_from_this());
      auto buff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      native_socket::async_receive(buff,
        [handler, this, self](const error_code& ec, size_t size) {
          ec ? handler(ec) : read_response(size, handler);
        }
      );
    }

    template <typename Handler>
    void async_ws_handshake(Handler&& handler) {
      error_code ec;
      if (is_client()) {
        send_request(ec);
        ec ? handler(ec) : read_response(handler);
      }
      else {
        read_request(handler);
      }
    }

  private:
    ws::opcode_type _opcode;
    std::list<std::string> _packets;
    const family _family;
    char _buffer[8192];
    bool _compress = false;
    bool _handshaked = false;

    void init_connect(const char* host, unsigned short port, const char* uri) {
      auto np = is_security() ? 443 : 80;
      std::string str(host);
      if (np != port) {
        str += ":";
        str += std::to_string(port);
      }
      if (uri) {
        request_header().uri = uri;
      }
      request_header().set_header("Host", str);
    }

    void on_timer(size_t expires) {
      if (is_websocket()) {
        if (_handshaked) {
          ping(nullptr, 0);
        }
      }
      native_socket::on_timer(expires);
    }

    ws::decoder   _decoder;
    ws::encoder   _encoder;
    http::request _request;
    http::request_parser _request_parser;
    http::response _response;
    http::response_parser _response_parser;

  public:
    ASIO_DECL static type_ref create(
      family what = family::native) {
      return std::make_shared<socket>(what);
    }

    ASIO_DECL static type_ref create(
      io_executor ios, family what = family::native) {
      return std::make_shared<socket>(ios, what);
    }

    ASIO_DECL static type_ref create(
      ssl::context& ctx, family what = family::native) {
#ifndef STDNET_USE_OPENSSL
      return type_ref();
#else
      return std::make_shared<socket>(ctx, what);
#endif
    }

    ASIO_DECL static type_ref create(
      io_executor ios, ssl::context& ctx, family what = family::native) {
#ifndef STDNET_USE_OPENSSL
      return type_ref();
#else
      return std::make_shared<socket>(ios, ctx, what);
#endif
    }

    ASIO_DECL socket(family what)
      : socket(service::local(), what) {
    }

    ASIO_DECL socket(ssl::context& ctx, family what)
      : socket(service::local(), ctx, what) {
    }

    ASIO_DECL socket(io_executor ios, family what)
      : native_socket(ios)
      , _opcode(ws::opcode_type::text)
      , _family(what)
      , _decoder(STDNET_MAX_PACKET) {
#ifdef _DEBUG
      memset(_buffer, 0, sizeof(_buffer));
#endif
    }

    ASIO_DECL socket(io_executor ios, ssl::context& ctx, family what)
      : native_socket(ios, ctx)
      , _opcode(ws::opcode_type::text)
      , _family(what)
      , _decoder(STDNET_MAX_PACKET) {
#ifdef _DEBUG
      memset(_buffer, 0, sizeof(_buffer));
#endif
    }

  public:
    ASIO_DECL native_socket& next_layer() {
      return *this;
    }

    ASIO_DECL bool is_native() const {
      return native_socket::is_native() && !is_websocket();
    }

    ASIO_DECL bool is_websocket() const {
      return _family == family::websocket;
    }

    ASIO_DECL http::response& response_header() {
      return _response;
    }

    ASIO_DECL http::request& request_header() {
      return _request;
    }

    ASIO_DECL endpoint_type remote_endpoint(error_code& ec) {
      endpoint_type peer;
      remote_endpoint(peer, ec);
      return peer;
    }

    ASIO_DECL void remote_endpoint(endpoint_type& peer, error_code& ec) {
      peer = native_socket::remote_endpoint(ec);
      if (ec) {
        return;
      }
      auto xfor = request_header().get_header("X-Forwarded-For");
      if (xfor.empty()) {
        return;
      }
      auto pos = xfor.find(',');
      if (pos != std::string::npos) {
        xfor = xfor.substr(0, pos);
      }
      auto addr = ip::make_address(xfor.c_str(), ec);
      if (!ec) {
        peer.address(addr);
      }
    }

    ASIO_DECL void close() {
      if (is_open()) {
        if (is_websocket()) {
          close(_decoder.lasterror());
        }
        else {
          native_socket::close();
        }
      }
    }

    ASIO_SYNC_OP_VOID handshake(error_code& ec) {
      native_socket::handshake(ec);
      if (!ec && is_websocket()) {
        ws_handshake(ec);
      }
    }

    ASIO_SYNC_OP_VOID connect(const char* host, unsigned short port, error_code& ec) {
      connect(host, port, nullptr, ec);
    }

    ASIO_SYNC_OP_VOID connect(const char* host, unsigned short port, const char* uri, error_code& ec) {
      native_socket::connect(host, port, ec);
      if (!ec && is_websocket()) {
        init_connect(host, port, uri);
      }
    }

    ASIO_SYNC_OP_VOID receive(std::string& out) {
      error_code ec;
      receive(out, ec);
    }

    ASIO_SYNC_OP_VOID receive(std::string& out, error_code& ec) {
      auto buff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      if (!is_websocket()) {
        size_t n = native_socket::receive(buff, ec);
        if (!ec) {
          out.assign(_buffer, n);
        }
        return;
      }
      if (next_packet(out).size()) {
        return;
      }
      while (out.empty()) {
        size_t n = native_socket::receive(buff, ec);
        if (ec) {
          return;
        }
        ec = decode_of(_buffer, n);
        if (ec) {
          return;
        }
        if (next_packet(out).size()) {
          return;
        }
      }
    }

    size_t send(const char* data, size_t size) {
      error_code ec;
      return send(data, size, ec);
    }

    size_t send(const char* data, size_t size, error_code& ec) {
      if (!is_websocket()) {
        return native_socket::send(data, size, ec);
      }
      auto opcode = make_op(data, size);
#ifdef STDNET_USE_DEFLATE
      std::string ziped;
      if (_compress) {
        zlib::deflate(data, size, ziped);
        data = ziped.c_str();
        size = ziped.size();
      }
#endif
      _encoder.encode(data, size, opcode, _compress,
        [&](const std::string& data, ws::opcode_type, bool) {
          native_socket::send(data.c_str(), data.size(), ec);
        }
      );
      return ec ? 0 : size;
    }

    template <typename Handler>
    void async_receive(Handler&& handler) {
      auto self(shared_from_this());
      auto buff = ASIO_BUFFER(_buffer, sizeof(_buffer));
      if (!is_websocket()) {
        native_socket::async_receive(buff,
          [handler, this, self](const error_code& ec, size_t size) {
            handler(ec, _buffer, size);
          }
        );
        return;
      }
      std::string out;
      if (next_packet(out).size()) {
        get_executor()->post(
          [handler, out]() {
            error_code ec;
            handler(ec, out.c_str(), out.size());
          }
        );
        return;
      }
      native_socket::async_receive(buff,
        [handler, this, self](const error_code& ec, size_t size) {
          if (ec) {
            handler(ec, nullptr, 0);
            return;
          }
          auto err = decode_of(_buffer, size);
          if (err) {
            handler(err, nullptr, 0);
            return;
          }
          async_receive(handler);
        }
      );
    }

    void async_send(const char* data, size_t size) {
      async_send(data, size, [](const error_code&, size_t) {});
    }

    template <typename Handler>
    void async_send(const char* data, size_t size, Handler&& handler) {
      if (!is_websocket()) {
        native_socket::async_send(data, size, handler);
        return;
      }
      auto opcode = make_op(data, size);
#ifdef STDNET_USE_DEFLATE
      std::string ziped;
      if (_compress) {
        zlib::deflate(data, size, ziped);
        data = ziped.c_str();
        size = ziped.size();
      }
#endif
      _encoder.encode(data, size, opcode, _compress,
        [&](const std::string& data, ws::opcode_type, bool) {
          native_socket::async_send(data.c_str(), data.size(), handler);
        }
      );
    }

    template <typename Handler>
    void async_handshake(Handler&& handler) {
      auto self(shared_from_this());
      native_socket::async_handshake(
        [handler, this, self](const error_code& ec) {
          (ec || !is_websocket()) ? handler(ec) : async_ws_handshake(handler);
        }
      );
    }

    template <typename Handler>
    void async_connect(const char* host, unsigned short port, Handler&& handler) {
      async_connect(host, port, nullptr, handler);
    }

    template <typename Handler>
    void async_connect(const char* host, unsigned short port, const char* uri, Handler&& handler) {
      init_connect(host, port, uri);
      native_socket::async_connect(host, port, handler);
    }
  }; //end of class
} //end of namespace detail

/***********************************************************************************/

template <typename Socket, typename Handler>
void async_receive(Socket peer, Handler&& handler) {
  peer->async_receive(
    [=](const error_code& ec, const char* data, size_t size) {
      pcall(handler, ec, data, size);
      if (!ec && peer->is_open()) {
        async_receive(peer, handler);
      }
    }
  );
}

template <typename Socket>
void sync_connect(Socket socket, const char* host, unsigned short port, const char* uri, error_code& ec) {
  socket->connect(host, port, ec);
  if (!ec) {
    socket->handshake(ec);
  }
}

template <typename Socket>
void sync_connect(Socket socket, const char* host, unsigned short port, error_code& ec) {
  sync_connect(socket, host, port, nullptr, ec);
}

template <typename Socket, typename Handler>
void async_connect(Socket socket, const char* host, unsigned short port, const char* uri, Handler&& handler) {
  socket->async_connect(host, port, [=](const error_code& ec) {
    ec ? handler(ec) : socket->async_handshake(handler);
  });
}

template <typename Socket, typename Handler>
void async_connect(Socket socket, const char* host, unsigned short port, Handler&& handler) {
  async_connect(socket, host, port, nullptr, handler);
}

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_WEBSOCKET_H
