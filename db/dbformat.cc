#include <stdio.h>
#include "db/dbformat.h"
#include "port/port.h"
#include "util/coding.h"

namespace leveldb{


    const char* InternalKeyComparator::Name()const{
        return "leveldb.InternalKeyComparator";
    }

    int InternalKeyComparator::Compare(const Slice& akey,const Slice& bkey){
        int r = user_comparator_->Compare(ExtractUserKey(akey),ExtractUserKey(bkey));
        if(r==0){
            const uint64_t anum = DecodeFixed64(akey.data()+akey.size()-8);
            const uint64_t bnum = DecodeFixed64(bkey.data()+bkey.size()-8);
            if(anum > bnum){
                r = -1;
            }else if(anum < bnum){
                r = 1;
            }
        }
        return r;
    }

    void  InternalKeyComparator::FindShortestSeparator(std::string* start,const Slice& limit)const{
        Slice user_start = ExtractUserKey(*start);
        Slice user_limit = ExtractUserKey(limit);
        std::string tmp(user_start.data(),user_start.size());
        user_comparator_.FindShortestSeparator(&tmp,user_limit);
        if(tmp.size()<user_start.size() && user_comparator_->Compare(user_start,tmp)<0){
            PutFixed64(&tmp,PackSequenceAndType(kMaxSequenceNumber,kValueForSeek));
            assert(this->Compare(*start,tmp)<0);
            assert(this->Compare(tmp,limit)<0);
            start->swap(tmp);
        }
    }

    void InternalKeyComparator::FindShortSuccessor(std::string* key)const{
        Slice user_key = ExtractUserKey(*key);
        std::string tmp(user_key.data(),user_key.size());
        user_comparator_->FindShortSuccessor(&tmp);
        if(tmp.size()<user_key.size() && user_comparator_->Compare(user_key,tmp)<0){
            PutFixed64(&tmp,PackSequenceAndType(kMaxSequenceNumber,kValueForSeek));
            assert(this->Compare(*key,tmp)<0);
            key->swap(tmp);
        }
    }
}

