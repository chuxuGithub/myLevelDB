#include "leveldb/table_builder.h"

#include <assert.h>
#include "leveldb/comparator.h"
#include "level/env.h"
#include "leveldb/filter_policy.h"
#include "leveldb/options.h"
#include "table/table_builder.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb{
    struct TableBuilder::Rep{
        Options options;
        Options index_block_options;
        WritableFile* file;
        uint64_t offset;
        Status status;
        BlockBuilder data_block;
        BlockBuilder index_block;
        std::string last_key;
        int64_t num_entries;
        bool closed;
        FilterBlockBuilder* filter_block;

        bool pending_index_entry;
    };
}
