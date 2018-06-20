#include "util/arena.h"
#include <assert.h>

namespace leveldb{
    static const int kBlockSize = 4096;

    Arena::Arena():memory_usage_(0){
        alloc_ptr_= nullptr;
        alloc_bytes_remaining_ = 0;
    }

    Arena::~Arena(){
        for(size_t i=0;i<block_.size();i++){
            delete[] block_[i];
        }
    }

    char* Arena::AllocFallBack(size_t bytes){
        if(bytes > kBlockSize/4){
            char* result = AllocNewBlock(bytes);
            return result;
        }

        alloc_ptr_ = AllocNewBlock(kBlockSize);
        alloc_bytes_remaining_ = kBlockSize;

        char* result = alloc_ptr_;
        alloc_ptr_ += bytes;
        alloc_bytes_remaining_ -= bytes;
        return result;
    }

    char* Arena::AllocNewBlock(size_t block_bytes){
        char* result = new char[block_bytes];
        block_.push_back(result);
        memory_usage_.NoBarrier_store(
                reinterpret_cast<void*>(MemoryUsage()+block_bytes_+sizeof(char*))
                );
        return result;
    }

    char* Arena::AllocateAligned(size_t bytes){
        const int align = (sizeof(void*)>8) ? sizeof(void*):8;
        assert(align&(align-1) == 0);
        size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_ & (align-1));
        size_t slop = current_mod==0? 0:align-current_mod;
        size_t needed = slop+bytes;

        char* result;
        if(needed <= alloc_bytes_remaining_){
            result = alloc_ptr_+slop;
            alloc_ptr_ += needed;
            alloc_bytes_remaining_ -= needed;
        }else{
            result = AllocFallBack(needed);

        }
        assert(reinterpret_cast<uintptr_t>(result)&(align-1) == 0);
        return result;
    }
}

