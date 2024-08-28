

#ifndef STDNET_IDENTIFIER_H
#define STDNET_IDENTIFIER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/os/configure.h"

/***********************************************************************************/

class distributor final {
  std::mutex _mutex;
  circular_buffer _pool;
  friend class identifier;

  inline distributor()
    : _pool(0xffff * sizeof(int)) {
    for (int i = 1; ; ++i) {
      if (!_pool.write(&i, sizeof(i))) {
        break;
      }
    }
  }

  typedef std::shared_ptr<
    distributor
  > type_ref;

  int pop() {
    int i = 0;
    std::unique_lock<std::mutex> lock(_mutex);
    _pool.read(&i, sizeof(i));
    return i;
  }

  void push(int i) {
    std::unique_lock<std::mutex> lock(_mutex);
    _pool.write(&i, sizeof(i));
  }

  inline static type_ref instance() {
    static type_ref _identifier(new distributor());
    return _identifier;
  }
};

class identifier final {
  distributor::type_ref _pool;
  const int _value;

public:
  inline identifier()
    : _pool(distributor::instance())
    , _value(_pool->pop()) {
  }
  inline ~identifier() {
    if (_value > 0) {
      _pool->push(_value);
    }
  }
  inline int value() const { return _value; }
};

/***********************************************************************************/

#endif //STDNET_IDENTIFIER_H
