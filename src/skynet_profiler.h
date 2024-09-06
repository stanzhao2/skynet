

#ifndef SKYNET_PROFILER_H
#define SKYNET_PROFILER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

/********************************************************************************/

void skynet_profiler(void* ptr, void* nptr, size_t osize, size_t nsize);

/********************************************************************************/

#endif //SKYNET_PROFILER_H
