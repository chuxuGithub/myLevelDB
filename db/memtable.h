#pragma once

#include <string>
#include "leveldb/db.h"
#include "db/dbformat.h"
#include "db/skiplist.h"
#include "util/arena.h"

namespace leveldb{
    class InternalKeyComparator;
    class MemTableIterator;

    class MemTable{
        public:
            explicit MemTable(const InternalKeyComparator& comparator);

            void Ref(){ref_++;}
            void UnRef(){
                ref_--;
                assert(ref_>=0);
                if(ref_<=0){
                    delete this;
                }
            }

            size_t ApproximateMemoryUsage();
            Iterator* NewIterator();

            void Add(SequenceNumber seq,ValueType type,const Slice& key,const Slice& value);
            bool Get(const LookupKey& key,std::string* value,Status* s);

        private:
            ~MemTable();

            struct keyComparator{
                const InternalKeyComparator comparator;
                explicit keyComparator(const InternalKeyComparator& c):comparator(c){}
                int operator()(const char* a,const char* b);
            };
            friend class MemTableIterator;
            friend class MemTableBackwardIterator;

            typedef SkipList<const char*,KeyComparator> Table;

            KeyComparator comparator_;
            int refs_;
            Arena arena_;
            Table table_;

            //不允许拷贝
            MemTable(const MemTable&);
            void operator=(const MemTable&);
    };
}
