

#ifndef STDNET_DECODER_H
#define STDNET_DECODER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/wss/protocols.h"

/*******************************************************************************/
namespace ws {
/*******************************************************************************/

class decoder final {
  const size_t max_packet;
  std::string _cache;
  ws::context _context;

public:
  inline decoder(size_t max_size = 0)
    : max_packet(max_size) {
    _context.mask = true;
  }
  inline void set_client() {
    _context.what = ws::session_type::client;
  }
  inline bool is_client() const {
    return _context.what == ws::session_type::client;
  }
  inline int lasterror() const {
    return _context.ec;
  }
  template <typename Handler>
  int decode(const char* data, size_t size, Handler&& handler) {
    if (size == 0) {
      return 0;
    }
    _cache.append(data, size);
    data = _cache.c_str();
    size = _cache.size();

    auto begin = data;
    auto end   = ws::decode(data, size, _context, handler);
    if (end == nullptr) {
      return _context.ec;
    }
    if (max_packet) {
      auto psize = _context.packet.size();
      if (psize > max_packet) {
        _context.ec = 1009;
        return _context.ec;
      }
    }
    size_t removed = (size_t)(end - begin);
    if (removed == size) {
      _cache.clear();
    }
    else if (removed) {
      _cache = _cache.substr(removed);
    }
    return 0;
  }
};

/*******************************************************************************/

class encoder final {
  ws::context _context;

public:
  inline encoder() {
    _context.mask = true;
  }
  inline void set_client() {
    _context.what = ws::session_type::client;
  }
  inline bool is_client() const {
    return _context.what == ws::session_type::client;
  }
  inline void disable_mask() {
    _context.mask = false;
  }
  template <typename Handler>
  void encode(const char* data, size_t size, opcode_type opcode, bool inflate, Handler handler) {
    _context.inflate = inflate;
    _context.opcode  = opcode;
    ws::encode(data, size, _context, handler);
  }
};

/*******************************************************************************/
} //end of namespace ws
/*******************************************************************************/

#endif //STDNET_DECODER_H
