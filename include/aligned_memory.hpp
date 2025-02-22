#pragma once
#include <memory.hpp>
#include <stdlib.h>


namespace M68K {

#if defined(_ISOC11_SOURCE)
#define aligned_alloc(alignment, size) aligned_alloc(alignment, size)
#elif defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
static inline void *aligned_alloc(size_t alignment, size_t size) {
    return _aligned_malloc(size, alignment);
}
static inline void aligned_free(void *ptr) {
    _aligned_free(ptr);
}
#else
// POSIX systems
#include <stdlib.h>
static inline void *aligned_alloc(size_t alignment, size_t size) {
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}
static inline void aligned_free(void *ptr) {
    free(ptr);
}
#endif
//////////////////////////////////////////////////////////////////////////



class AlignedMemory final : public BaseMemory {
public:
    AlignedMemory() : BaseMemory(aligned_alloc(MEMORY_SIZE, MEMORY_SIZE), MEMORY_SIZE) {
    }
    virtual ~AlignedMemory() {
        aligned_free(BaseMemory::baseAddr);
    }

};  // class MemoryVector
//////////////////////////////////////////////////////////////////////////


};  // namespace M68K
