/*************************************************************************
	> File Name: table_builder.h
	> Author: 
	> Mail: 
	> Created Time: Mon 02 Jul 2018 07:11:19 AM EDT
 ************************************************************************/

#ifndef _TABLE_BUILDER_H
#define _TABLE_BUILDER_H

#include <stdint.h>
#include "leveldb/export.h"
#include "leveldb/options.h"
#include "leveldb/status.h"

namespace leveldb{
    class BlockBulider;
    class BlockHandle;
    class WritableFile;

    class LEVEDB_EXPORT TableBuilder{
        public:
        TableBuilder(const Options& options,WritableFile* file);
        TableBuilder(const TableBuilder&)=delete;
        void operator=(const TableBuilder&)=delete;
        ~TableBuilder();

        Status ChangeOptions(const Options& options);
        void Add(const Slice& key,const Slice& value);
        void Flush();
        Status status()const;
        Status Finish();
        void Abandon();
        uint64_t  NumEntries()const;
        uint64_t FileSize()const;

        private:
        bool ok()const{return status().ok();}
        void WriteBlock(BlockBuilder* block,BlockHand* handle);
        void WriteRawBlock(const Slice& data,CompressionType,BlockHandle* handle);

        struct Rep;
        Rep* rep_;
    };
}

#endif
