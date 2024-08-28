

#include <stdlib.h>
#include <string.h>
#include "skynet_allotor.h"

/********************************************************************************/

#define sizeof_array 64
#define sizeof_cache 4096

#define sizeof_pointer sizeof(void*)
#define skynet_min(a, b) ((a) < (b) ? (a) : (b))

/********************************************************************************/

class lua_allotor final {
  class mallotor {
  public:
    mallotor();
    ~mallotor();
    void* pop();
    bool  push(void*);

  private:
    size_t ri, wi;
    char mp[sizeof_cache];
  };

public:
  lua_allotor();
  ~lua_allotor();
  void  clear();
  void  p_free(void* ptr, size_t os);
  void* p_realloc(void* ptr, size_t os, size_t ns);

private:
  mallotor* allotor_array[sizeof_array];
};

/********************************************************************************/

lua_allotor::mallotor::mallotor()
  : ri(0), wi(0) {
  memset(mp, 0, sizeof(mp));
}

lua_allotor::mallotor::~mallotor() {
  while (wi - ri) free(pop());
}

void* lua_allotor::mallotor::pop() {
  void* p = 0;
  size_t n = skynet_min(sizeof_pointer, wi - ri);
  if (n) {
    size_t i = (ri & (sizeof_cache - 1));
    *(size_t*)(&p) = *(size_t*)(mp + i);
    ri += n;
  }
  return p;
}

bool lua_allotor::mallotor::push(void* p) {
  size_t n = skynet_min(sizeof_pointer, sizeof_cache - (wi - ri));
  if (n) {
    size_t i = (wi & (sizeof_cache - 1));
    *(size_t*)(mp + i) = (size_t)p;
    wi += n;
  }
  return n == sizeof_pointer;
}

/********************************************************************************/

lua_allotor::lua_allotor() {
  for (int i = 0; i < sizeof_array; i++) {
    allotor_array[i] = new mallotor();
  }
}

lua_allotor::~lua_allotor() {
  clear();
}

void lua_allotor::clear() {
  for (int i = 0; i < sizeof_array; i++) {
    delete allotor_array[i];
    allotor_array[i] = nullptr;
  }
}

void lua_allotor::p_free(void* ptr, size_t os) {
  if (ptr) {
    auto oi = (os - 1) >> 4;
    if (oi >= sizeof_array || !allotor_array[oi]->push(ptr)) {
      free(ptr);
    }
  }
}

void* lua_allotor::p_realloc(void* ptr, size_t os, size_t ns) {
  auto oi = (os - 1) >> 4;
  auto ni = (ns - 1) >> 4;
  if (ptr && oi == ni) {
    return ptr;
  }
  void* p = 0;
  if (ni < sizeof_array) {
    p = allotor_array[ni]->pop();
  }
  if (p && ptr) {
    memcpy(p, ptr, skynet_min(os, ns));
    if (oi >= sizeof_array || !allotor_array[oi]->push(ptr)) {
      free(ptr);
    }
  }
  return p ? p : realloc(ptr, (ni + 1) << 4);
}

/********************************************************************************/

void* skynet_allotor(void* ud, void* ptr, size_t osize, size_t nsize) {
  static thread_local lua_allotor allotor;
  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    //free(ptr);
    allotor.p_free(ptr, osize);
    return NULL;
  }
  //return realloc(ptr, nsize);
  return allotor.p_realloc(ptr, osize, nsize);
}

/********************************************************************************/
