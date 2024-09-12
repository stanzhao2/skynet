

#include <stdlib.h>
#include <string.h>
#include "skynet_allotor.h"

/********************************************************************************/

#define sizeof_array 16 /* max 1024 bytes */
#define sizeof_cache (sizeof(size_t) * 8192)

#define size_of_ptr sizeof(void*)
#define size_of_min(a, b) ((a) < (b) ? (a) : (b))
#define index_of_size(n) ((n - 1) >> 6) /* base on 64 bytes */
#define size_of_index(i) ((i + 1) << 6) /* base on 64 bytes */

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

static thread_local lua_allotor allotor;

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
  size_t n = size_of_min(size_of_ptr, wi - ri);
  if (n) {
    size_t i = (ri & (sizeof_cache - 1));
    *(size_t*)(&p) = *(size_t*)(mp + i);
    ri += n;
  }
  return p;
}

bool lua_allotor::mallotor::push(void* p) {
  size_t n = size_of_min(size_of_ptr, sizeof_cache - (wi - ri));
  if (n) {
    size_t i = (wi & (sizeof_cache - 1));
    *(size_t*)(mp + i) = (size_t)p;
    wi += n;
  }
  return n == size_of_ptr;
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
    auto oi = index_of_size(os);
    if (oi >= sizeof_array || !allotor_array[oi]->push(ptr)) {
      free(ptr);
    }
  }
}

void* lua_allotor::p_realloc(void* ptr, size_t os, size_t ns) {
  auto oi = index_of_size(os);
  auto ni = index_of_size(ns);
  if (ptr && oi == ni) {
    return ptr;
  }
  void* new_ptr = nullptr;
  if (ni < sizeof_array) {
    new_ptr = allotor_array[ni]->pop();
  }
  if (ptr && new_ptr) {
    memcpy(new_ptr, ptr, size_of_min(os, ns));
    if (oi >= sizeof_array || !allotor_array[oi]->push(ptr)) {
      free(ptr);
    }
  }
  return new_ptr ? new_ptr : realloc(ptr, size_of_index(ni));
}

/********************************************************************************/

void* skynet_allotor(void* ud, void* ptr, size_t osize, size_t nsize) {  
  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    allotor.p_free(ptr, osize);
    return NULL;
  }
  return allotor.p_realloc(ptr, osize, nsize);
}

/********************************************************************************/
