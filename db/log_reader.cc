#include <cassert>
#include <string>
#include "log_reader.h"
#include "logformat.h"
#include "env.h"
#include "status.h"
#include "../util/crc32c.h"

namespace leveldb_clone {

namespace log {

// return a record from a loaded block
// [crc32 4bytes | len 2bytes | vtype 1bytes | payload]
RecordType Reader::ReadPhysicalRecord(std::string** scatch) {
    // Only return one record
    bool space = offset_ + 7 <= kBlockSize;
    if (!space) {
        size_t n = 0;
        Status s = seqfile_->Read(buf_, kBlockSize, &n);
        if (!s.IsOk())
            return kZeroType;
        block_ += 1;
        offset_ = 0;
    }
    uint8_t type = static_cast<uint8_t>(buf_[offset_ + 6]);
    uint16_t len = static_cast<uint16_t>(buf_[offset_ + 4]) | (static_cast<uint16_t>(buf_[offset_ + 5]) << 8);
    char* payload = &buf_[offset_ + 7];
    uint32_t checksum = crc32c::Crc32(type, payload, len);

    char crc32[4];
    memcpy(crc32, &buf_[offset_], 4);
    if (checksum != DecodeFixed32(crc32))
        return RecordType::kZeroType;

    (*scatch)->append(payload, len);
    offset_ += 7 + len;
    return static_cast<RecordType>(type);
}

bool Reader::ReadRecord(Slice* record, std::string* scatch) {
    (*scatch).clear();

    // Get a record until its type is Full or Last
    while (true) {
        RecordType type = ReadPhysicalRecord(&scatch);
        bool flag = false;
        switch (type)
        {
        case RecordType::kFullType:
        case RecordType::kLastType:
            flag = true;
            break;
        case RecordType::kFirstType:
        case RecordType::kMiddleType:
            continue;
        case RecordType::kZeroType:
        default:
            return false;
        }
        if (flag)
            break;
    }
    *record = Slice{*scatch};
    return true;
}


} // namespace log

} // namespace leveldb_clone