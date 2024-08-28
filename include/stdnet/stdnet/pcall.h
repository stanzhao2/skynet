

#ifndef STDNET_PCALL_H
#define STDNET_PCALL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/config.h"
#include "stdnet/namespace.h"

/***********************************************************************************/

#define pcall(f, ...) try { \
  f(__VA_ARGS__);  \
}                         \
catch(const std::exception& e) { \
  printf("%s exception: %s\n", __FUNCTION__, e.what()); \
}

/***********************************************************************************/

#endif //STDNET_PCALL_H
