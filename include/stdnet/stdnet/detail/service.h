

#ifndef STDNET_SERVICE_H
#define STDNET_SERVICE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/identifier.h"
#include "stdnet/pcall.h"
#include "stdnet/namespace.h"

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

namespace detail {
  class service : public io_context
    , public std::enable_shared_from_this<service> {
    const identifier _id;
    signal_set  _signal;
    semaphore   _semaphore;
    const void* _context = nullptr;
    executor_work_guard<executor_type> work_guard;

  public:
    typedef std::shared_ptr<
      service
    > type_ref;

    ASIO_DECL explicit service(int concurrency_hint);

    template <typename Handler>
    ASIO_DECL void post(Handler&& handler);

    template <typename Handler>
    ASIO_DECL void dispatch(Handler&& handler);

  public:
    ASIO_DECL service();
    ASIO_DECL int  id() const;
    ASIO_DECL void wakeup();
    ASIO_DECL void wait();
    ASIO_DECL bool wait_for(size_t expires);
    ASIO_DECL void set_context(const void* context);
    ASIO_DECL const void* get_context() const;
    ASIO_DECL signal_set& signal();
    ASIO_DECL static type_ref local();
    ASIO_DECL static type_ref create();
    ASIO_DECL static type_ref create(int concurrency_hint);
  };

  ASIO_DECL service::service()
    : io_context()
    , _signal(*this)
    , work_guard(make_work_guard(*this)) {
  }

  ASIO_DECL service::service(int concurrency_hint)
    : io_context(concurrency_hint)
    , _signal(*this)
    , work_guard(make_work_guard(*this)) {
  }

  ASIO_DECL const void* service::get_context() const {
    return _context;
  }

  ASIO_DECL void service::set_context(const void* context) {
    _context = context;
  }

  ASIO_DECL int service::id() const {
    return _id.value();
  }

  ASIO_DECL void service::wakeup() {
    auto self(shared_from_this());
    this->post([this, self]() { _semaphore.signal(); });
  }

  ASIO_DECL void service::wait() {
    while (!stopped()) {
      run_one();
      if (_semaphore.wait_for(0)) {
        _semaphore.clear();
        break;
      }
    }
  }

  ASIO_DECL bool service::wait_for(size_t expires) {
    auto begin = steady_clock();
    while (!stopped()) {
      auto now = steady_clock();
      if (now - begin >= expires) {
        break;
      }
      run_one_for(std::chrono::milliseconds(expires));
      if (_semaphore.wait_for(0)) {
        _semaphore.clear();
        return true;
      }
    }
    return false;
  }

  template <typename Handler>
  ASIO_DECL void service::post(Handler&& handler) {
    asio::post(*this, handler);
  }

  template <typename Handler>
  ASIO_DECL void service::dispatch(Handler&& handler) {
    asio::dispatch(*this, handler);
  }

  ASIO_DECL service::type_ref service::local() {
    static thread_local type_ref _local = create();
    return _local;
  }

  ASIO_DECL signal_set& service::signal() {
    return _signal;
  }

  ASIO_DECL service::type_ref service::create() {
    return std::make_shared<service>();
  }

  ASIO_DECL service::type_ref service::create(int concurrency_hint) {
    return std::make_shared<service>(concurrency_hint);
  }
} //end of namespace detail

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_SERVICE_H
