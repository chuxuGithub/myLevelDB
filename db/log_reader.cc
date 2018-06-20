#include "db/log_reader.h"
#include <stdio.h>
#include "leveldb/env.h"
#include "util/coding.h"
#include "util/crc43c.h"

namespace leveldb{
    namespace log{
        Reader::Reporter::~Reporter(){}

        Reader::Reader(SequentialFile* file,Reporter* reporter,bool checksum,uint64_t initial_offset);
            file_(file),
            reporter_(reporter),
            checksum_(checksum),
            backing_store_(new char[kBlockSize]),
            buffer_(),
            eof(false),
            last_record_offset_(0),
            end_of_buffer_offset_(0),
            initial_offset_(initial_offset),
            resyncing_(initial_offset>0){

        }

        Reader::~Reader(){
            delete[] backing_store_;
        }

        bool Reader::SkipToInitialBlock(){
            const size_t offset_in_block = initial_offset_%kBlockSize;
            uint64_t block_start_location = initial_offset_ - offset_in_block;
            if(offset_in_block > kBlockSize-6){
                block_start_location += kBlockSize;
            }

            end_of_buffer_offset_ = block_start_location;

            if(block_start_location > 0){
                Status skip_status = file_->skip(block_start_location);
                if(!skip_status.ok()){
                    ReportDrop(block_start_location,skip_status);
                    return false;
                }
            }
            return true;
        }

        void Reader::ReadRecord(Slice* record,std::string* scratch){
            if(last_record_offset_ < initial_offset_){
                if(!SkipToInitialBlock()){
                    return false;
                }
            }

            scratch->clear();
            record->clear();

            bool in_fragmented_record = false;
            uint64_t prospective_record_offset = 0;

            Slice fragment;
            while(true){
                const unsigned int record_type = ReadPhysicalRecord(&fragment);
                uint64_t physical_record_offset = end_of_buffer_offset_ - buffer_.size() - kHeadSize - fragment.size();

                if(resyncing_){
                    if(record_type == kMiddleType){
                        continue;
                    }else if(record_type == kLastType){
                        resyncing_ = false;
                        continue;
                    }else{
                        resyncing_ = false;
                    }
                }

                switch(record_type){
                    case kFullType:
                        if(in_fragmented_record){
                            if(!scratch->empty()){
                                //ReportCorruption
                            }
                            prospective_record_offset = physical_record_offset;
                            scratch->clear();
                            *record = fragment;
                            last_record_offset_ = prospective_record_offset;
                            return true;
                        }
                    case kFirstType:
                        if(in_fragmented_record){
                            if(!scratch->empty()){
                                //ReportCorruption
                            }
                        }
                        prospective_record_offset = physical_record_offset;
                        scratch->assign(fragment.data(),fragment.size());
                        in_fragmented_record = true;
                        break;
                    case kMiddleType:
                        if(!in_fragmented_record){
                            //ReportCorruption
                        }else{
                            scratch->append(fragment.data(),fragment.size());
                        }
                        break;

                    case kLastType:
                        if(!in_fragmented_record){
                            //ReportCorruption
                        }else{
                            scratch->append(fragment.data(),fragment.size());
                            *record = Slice(*scratch);
                            last_record_offset_ = prospective_record_offset;
                            return true;
                        }
                        break;
                    case kEof:
                        if(in_fragmented_record){
                            scratch->clear();
                        }
                        return false;
                    case kBadRecord:
                        if(in_fragmented_record){
                            //ReportCorruption
                            in_fragmented_record = false;
                            scratch->clear();
                        }
                        break;
                    default:
                        char buf[40];
                        //snprintf
                        //ReportCorruption
                        scratch->clear();
                        break;
                }
            }
            return false;
        }


        uint64_t Reader::LastRecordOffset(){
            return last_record_offset_;
        }

        void Reader::ReportCorruption(uint64_t bytes,const char* reason){
            ReportDrop(bytes,Status::Corruption(reason));
        }

        void Reader::ReportDrop(uint64_t bytes,const Status& reason){
            if(reporter_!=nullptr && end_of_buffer_offset_-buffer_.size()-bytes >= initial_offset_){
                reporter_->Corruption(static_cast<size_t>(bytes),reason);
            }
        }

        unsigned int Reader::ReadPhysicalRecord(Slice* result){
            while(true){
                if(buffer_.size() < kHeadSize){
                    if(!eof){
                        buffer_.clear();
                        Status status = file_->Read(kBlockSize.&buffer_,backing_store_);
                        end_of_buffer_offset_ += buffer_.size();
                        if(!status.ok()){
                            buffer_.clear();
                            ReportDrop(kBlockSize,status);
                            eof_ = true;
                            return kEof;
                        }else if(buffer_.size() < kBlockSize)
                            eof_ = true;

                    }
                    continue;
                }else{
                    buffer_.clear();
                    return kEof;
                }
            }

            const char* header = buffer_.data();
            const uint32_t a = static_cast<uint32_t>(header[4]) & 0xff;
            const uint32_t b = static_cast<uint32_t>(header[5]) & 0xff;
            const unsigned int type = header[6];
            const uint32_t length = a | (b<<8);
            if(kHeadSize+length > buffer_.size()){
                size_t = drop_size = buffer_.size();
                buffer_.clear();
                if(!eof_){
                    //ReportCorruption
                    return kBadRecord;
                }
                return kEof;
            }
            if(type==kZeroType && length==0){
                buffer_.clear();
                return kBadRecord;
            }

            if(checksum_){
                uint32_t expected_crc = crc32c::Unmask(DecodeFixed32(header));
                uint32_t actual_crc = crc32c::Value(header+6,1+length);
                if(actual_crc != expected_crc){
                    size_t drop_size = buffer_.size();
                    buffer_.clear();
                    //ReportCorruption
                    return kBadRecord;
                }
            }

            buffer_.remove_prefix(kHeadSize+length);

            if(end_of_buffer_offset_-buffer_.size()-kHeadSize-length < initial_offset_){
                result->clear();
                return kBadRecord;
            }

            *result = Slice(header+kHeadSize,length);
            return type;
        }


    }
}
