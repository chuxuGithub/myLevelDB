#include <stdio.h>
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/slice.h"
#include "leveldb/table_builder.h"
#include "util/coding.h"
#include "util/logging.h"

namespace leveldb{
    namespace config{
        static const int kNumLevels = 7;
        static const int kL0_CompactionTrigger = 4;
        static const int kL0_SlowdownWritesTrigger = 8;
        static const int kL0_StopWritesTrigger = 12;

        static const int kMaxMemCompactLevel = 2;
        static const int kReadBytesPeriod = 1048576;
    }

    class InternalKeyComparator : public Comparator{
        private:
            const Comparator* user_comparator_;

        public:
            explicit InternalKeyComparator(const Comparator* c):user_comparator_(c){}
            virtual const char* Name()const;
            virtual int Compare(const Slice& a,const Slice& b)const;
            virtual void FindShortestSeparator(std::string* start,const Slice& limit)const;
            virtual void FindShortSuccssor(std::string* key)const;
            const Comparator* user_comparator()const{return user_comparator_;}

            int Compare(const InternalKey& a,const InternalKey& b)const;
    };


}
