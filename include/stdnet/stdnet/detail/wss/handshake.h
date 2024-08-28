

#ifndef STDNET_HANDSHAKE_H
#define STDNET_HANDSHAKE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/wss/parser.h"
#include "stdnet/detail/algo/sha1.h"
#include "stdnet/detail/algo/base64.h"

namespace ws {
#if defined(ZLIB_CONST)
  inline bool use_extensions() { return true; }
#else
  inline bool use_extensions() { return false; }
#endif

  inline bool deflate_enable(const http::response& res) {
    auto ext = res.get_header("Sec-WebSocket-Extensions");
    return use_extensions() && !ext.empty();
  }

  inline std::string request_key(std::string key) {
    char data[128];
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    key = hash_sha1(key);
    base64::encode((unsigned char*)key.c_str(), data, (int)key.size());
    key.assign(data);
    return key;
  }

  inline std::string request_key() {
    return request_key(std::to_string(system_clock()));
  }

  inline void init_request(http::request& req) {
    req.method = "GET";
    req.http_version_major = 1;
    req.http_version_minor = 1;

    if (req.get_header("User-Agent").empty()) {
      req.set_header("User-Agent", "Websocket " __TIMESTAMP__);
    }
    req.set_header("Cache-Control", "no-cache");
    req.set_header("Pragma", "no-cache");
    req.set_header("Connection", "Upgrade");
    req.set_header("Upgrade", "websocket");
    req.set_header("Sec-WebSocket-Key", request_key());
    req.set_header("Sec-WebSocket-Version", WS_VERSION);

    if (use_extensions()) {
      req.set_header("Sec-WebSocket-Extensions", "permessage-deflate; client_max_window_bits");
    }
  }

  inline void init_response(int status, http::response& res, const http::request& req) {
    std::string key;
    if (status == 101) {
      if (!http::is_ws_request(req)) {
        status = 400;
      }
      else {
        key = req.get_header("Sec-WebSocket-Key");
        if (key.empty()) {
          status = 400;
        }
      }
    }
    res.status = status;
    res.http_version_major = 1;
    res.http_version_minor = 1;
    if (status != 101) {
      res.set_header("Cache-Control", "max-age=0");
      res.set_header("Pragma", "no-cache");
      res.set_header("Connection", "Close");
      return;
    }
    auto origin = req.get_header("Origin");
    if (!origin.empty()) {
      res.set_header("Access-Control-Allow-Credentials", "true");
      res.set_header("Access-Control-Allow-Origin", origin);
    }
    if (use_extensions()) {
      auto ext = req.get_header("Sec-WebSocket-Extensions");
      if (ext.find("permessage-deflate") == 0) {
        res.set_header("Sec-WebSocket-Extensions", "permessage-deflate; client_no_context_takeover; server_max_window_bits=15");
      }
    }
    res.set_header("Connection", "Upgrade");
    res.set_header("Upgrade", "websocket");
    res.set_header("Sec-WebSocket-Accept", request_key(key));
  }

  inline bool check_response(const http::response& res, const http::request& req) {
    auto key = req.get_header("Sec-WebSocket-Key");
    auto accept_key = res.get_header("Sec-WebSocket-Accept");
    return (res.status == 101 && accept_key == request_key(key));
  }
}

#endif //STDNET_HANDSHAKE_H
