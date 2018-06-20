#pragma once

#include <string>

namespace leveldb{
    class Slice;

    class LEVELDB_EXPORT Comparator{
        public:
            virtual ~Comparator();
            virtual int Comparator(const Slice& a,const Slice& b)const=0;
            virtual char* Name()const=0;
            virtual void FindShortestSeparator(const std::string* start,const Slice& limit)const=0;
            virtual void FindShortSuccessor(std::string* key)const=0;
    };

    LEVELDB_EXPORT const Comparator* ByteWiseComparator();
}
