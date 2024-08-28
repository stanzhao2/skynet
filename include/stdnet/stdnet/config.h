

#ifndef STDNET_CONFIG_H
#define STDNET_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#if defined(_DEBUG) || defined(DEBUG)
#define STDNET_DEBUG
#endif

#include "stdnet/os/configure.h"

/***********************************************************************************/

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef ASIO_NO_DEPRECATED
#define ASIO_NO_DEPRECATED
#endif

#include <asio.hpp> //using asio c++ library
using namespace asio;

#ifdef STDNET_USE_OPENSSL
#include <asio/ssl.hpp>
#endif

#define ASIO_BUFFER  asio::buffer
#define STDNET_ERROR asio::error::access_denied

#ifdef STDNET_USE_DEFLATE
#include "stdnet/detail/wss/deflate.h"
#endif

/***********************************************************************************/

#endif //STDNET_CONFIG_H
