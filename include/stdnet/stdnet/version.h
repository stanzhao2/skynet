

#ifndef STDNET_VERSION_H
#define STDNET_VERSION_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

// STDNET_VERSION % 100 is the sub-minor version
// STDNET_VERSION / 100 % 1000 is the minor version
// STDNET_VERSION / 100000 is the major version
#define STDNET_VERSION 103002 // 1.30.2

#endif // STDNET_VERSION_H
