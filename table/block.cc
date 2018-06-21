#include "table/block.h"

#include <vector>
#include <algorithm>
#include "leveldb/comparator.h"
#include "table/format.h"
#include "util/coding.h"
#include "util/logging.h"

namespace leveldb{
    inline uint32_t Block::NumRestarts()const{
        assert(size_ >= sizeof(uint32_t));
        return DecodeFixed32(data_+size_-sizeof(uint32_t));
    }

    Block::Block(const BlockContents& contents):
        data_(contents.data.data()),
        size_(contents.data.size()),
        owned_(contents.heap.alloc){
            if(size_ < sizeof(uint32_t)){
                size_ = 0;
            }else{
                size_t max_restarts_allowed = (size_-sizeof(uint32_t)) / sizeof(uint32_t);
                if(NumRestarts() > max_restarts_allowed){
                    size_ = 0;
                }else{
                    restart_offset_ = size_ - (1+NumRestarts())*sizeof(uint32_t);
                }
            }
        }

    Block::~Block(){
        if(owned_){
            delete[] data_;
        }
    }

    static inline const char* DecodeEntry(const char* p,const char* limit,uint32_t* shared,uint32_t* non_shared,uint32_t value_length){
        if(limit-p < 3){
            return nullptr;
        }
        *shared = reinterpret_cast<const unsigned char*>(p)[0];
        *non_shared = reinterpret_cast<const unsigned char*>(p)[1];
        *value_length = reinterpret_cast<const unsigned char*>(p)[2];

        if((*shared|*non_shared|*value_length) < 128){
            p += 3;
        }else{
            if((p=GetVarint32Ptr(p,limit,shared)) == nullptr) return nullptr;
            if((p=GetVarint32Ptr(p,limit,non_shared)) == nullptr) return nullptr;
            if((p=GetVarint32Ptr(p,limit,value_length) == nullptr)) return nullptr;


        }

        if(static_cast<uint32_t>(limit-p) < (*non_shared+*value_length)){
            return nullptr;
        }

        return p;
    }

    class Block::Iter : public Iterator{
        private:
            const Comparator* const comparator_;
            const char* const data_;
            uint32_t const restarts_;
            uint32_t const num_restarts_;

            uint32_t current_;
            uint32_t restart_index_;
            std::string key_;
            Slice value;
            Status status_;


            inline int Compare(const Slice& a,const Slice& b)const{
                return comparator_.Compare(a,b);
            }

            inline uint32_t NextEntryOffset()const{
                return (value_.data()+value_.size())-data_;
            }

            uint32_t GetRestartPoint(uint32_t index){
                assert(index < num_restarts_);
                return DecodeFixed32(data_ + restarts_ + index*sizeof(uint32_t));
            }

            void SeekToRestartPoint(uint32_t index){
                key_.clear();
                restart_index_ = index;
                uint32_t offset = GetRestartPoint(index);
                value_ = Slice(data_+offset,0);
            }

        public:
            Iter(const Comparator* comparator,
                    const char* data,
                    uint32_t restarts,
                    uint32_t num_restarts):
                comparator_(comparator),
                data_(data),
                restarts_(restarts),
                num_restarts_(num_restarts),
                current_(restarts),
                restarts_index_(num_restarts){
                    assert(num_restarts_ > 0);
                }

            virtual bool Valid ()const {return current_ < restarts_;}

            virtual Status status()const{return status_;}

            virtual Slice Key()const{
                assert(Valid());
                return key_;
            }

            virtual Slice Value()const{
                assert(Valid());
                return value_;
            }
    };
}
