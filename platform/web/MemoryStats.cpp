#include "util/MemoryStats.h"

#include <emscripten/heap.h>

namespace RBX::MemoryStats {

memsize_t usedMemoryBytes()
{
    return static_cast<memsize_t>(emscripten_get_heap_size());
}

memsize_t totalMemoryBytes()
{
    return static_cast<memsize_t>(emscripten_get_heap_max());
}

memsize_t freeMemoryBytes()
{
    const memsize_t total = totalMemoryBytes();
    const memsize_t used = usedMemoryBytes();
    return total > used ? total - used : 0;
}

}
