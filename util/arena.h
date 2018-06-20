#pragma once

#include <vector>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "port/port.h"

namespace leveldb{
    class Arena{
        public:
            Arena();
            ~Arena();

            char* Allocate(size_t bytes);
            char* AllocateAligned(size_t bytes);
            size_t MemoryUsage() const{
                return reinterpret_cast<uintptr_t>(memory_usage_.NoBarrier_Load());
            }

        private:
            char* AllocateFallBack(size_t bytes);
            char* AllocateNewBlock(size_t block_bytes);

            char* alloc_ptr_;
            size_t alloc_bytes_remaining_;

            std::vector<char*> blocks_;

            port::AtomicPointer memory_usage_;

            Arena(const Arena&);
            void operator=(const Arena&);
    };

    inline char* Allocate(size_t bytes){
        assert(bytes>0);
        if(bytes <= alloc_bytes_remaining_){
            char* result = alloc_ptr_;
            alloc_bytes_remaining_ -= bytes;
            alloc_ptr_ += bytes;
            return result;
        }
        return AllocateFallBack(bytes);
    }
}
