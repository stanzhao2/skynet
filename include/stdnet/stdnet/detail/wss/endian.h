

#ifndef STDNET_ENDIAN_H
#define STDNET_ENDIAN_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "stdnet/pcall.h"

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

/*******************************************************************************/

inline char* ep_encode8(char *p, u8 c) {
  *(u8*)p++ = c;
  return p;
}

inline char* ep_encode16(char* p, u16 w) {
  ((u8*)p)[0] = ((u8*)&w)[1];
  ((u8*)p)[1] = ((u8*)&w)[0];
  return p + 2;
}

inline char* ep_encode32(char *p, u32 i) {
  ((u8*)p)[0] = ((u8*)&i)[3];
  ((u8*)p)[1] = ((u8*)&i)[2];
  ((u8*)p)[2] = ((u8*)&i)[1];
  ((u8*)p)[3] = ((u8*)&i)[0];
  return p + 4;
}

inline char* ep_encode64(char *p, u64 l) {
  ((u8*)p)[0] = ((u8*)&l)[7];
  ((u8*)p)[1] = ((u8*)&l)[6];
  ((u8*)p)[2] = ((u8*)&l)[5];
  ((u8*)p)[3] = ((u8*)&l)[4];
  ((u8*)p)[4] = ((u8*)&l)[3];
  ((u8*)p)[5] = ((u8*)&l)[2];
  ((u8*)p)[6] = ((u8*)&l)[1];
  ((u8*)p)[7] = ((u8*)&l)[0];
  return p + 8;
}

/*******************************************************************************/

inline const char* ep_decode8(const char *p, u8* c) {
  *c = *(u8*)p++;
  return p;
}

inline const char* ep_decode16(const char *p, u16* w) {
  ((u8*)w)[1] = ((u8*)p)[0];
  ((u8*)w)[0] = ((u8*)p)[1];
  return p + 2;
}

inline const char* ep_decode32(const char *p, u32* i) {
  ((u8*)i)[3] = ((u8*)p)[0];
  ((u8*)i)[2] = ((u8*)p)[1];
  ((u8*)i)[1] = ((u8*)p)[2];
  ((u8*)i)[0] = ((u8*)p)[3];
  return p + 4;
}

inline const char *ep_decode64(const char *p, u64* l) {
  ((u8*)l)[7] = ((u8*)p)[0];
  ((u8*)l)[6] = ((u8*)p)[1];
  ((u8*)l)[5] = ((u8*)p)[2];
  ((u8*)l)[4] = ((u8*)p)[3];
  ((u8*)l)[3] = ((u8*)p)[4];
  ((u8*)l)[2] = ((u8*)p)[5];
  ((u8*)l)[1] = ((u8*)p)[6];
  ((u8*)l)[0] = ((u8*)p)[7];
  return p + 8;
}

#endif //STDNET_ENDIAN_H
