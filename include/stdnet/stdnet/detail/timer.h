

#ifndef STDNET_TIMER_H
#define STDNET_TIMER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/detail/service.h"

/***********************************************************************************/
STDNET_NAMESPACE_BEGIN
/***********************************************************************************/

namespace detail {
  namespace steady {
    class timer : public steady_timer {
      service::type_ref _ios;

    public:
      typedef std::shared_ptr<
        timer
      > type_ref;

    public:
      ASIO_DECL timer(service::type_ref ios)
        : steady_timer(*ios)
        , _ios(ios) {
      }

      ASIO_DECL service::type_ref get_executor() const {
        return _ios;
      }

      ASIO_DECL static type_ref create(service::type_ref ios) {
        return std::make_shared<timer>(ios);
      }
    };
  }

  namespace system {
    class timer : public system_timer {
      service::type_ref _ios;

    public:
      typedef std::shared_ptr<
        timer
      > type_ref;

    public:
      ASIO_DECL timer(service::type_ref ios)
        : system_timer(*ios)
        , _ios(ios) {
      }

      ASIO_DECL service::type_ref get_executor() const {
        return _ios;
      }

      ASIO_DECL static type_ref create(service::type_ref ios) {
        return std::make_shared<timer>(ios);
      }
    };
  }
}

/***********************************************************************************/
STDNET_NAMESPACE_END
/***********************************************************************************/

#endif //STDNET_TIMER_H
