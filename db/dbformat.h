#include <string>
#include <stdint.h>
#include <cassert>
#include "slice.h"

#ifndef LEVELDB_CLONE_DB_DBFORMAT_H
#define LEVELDB_CLONE_DB_DBFORMAT_H

namespace leveldb_clone {

using SequenceNumber = uint64_t;


enum ValueType { kTypeDeletion = 0x0, kTypeValue = 0x1 };

// static char* EncodeVarint32

class InternalKey {
public:
    InternalKey() = default;

    // Takes in key, seq, valuetype and encoded them
    InternalKey(const Slice& key, SequenceNumber s, ValueType type) {
        rep_.append(key.data(), key.size());
        uint64_t tmp = (s << 8) | type; 
        // Turn tmp into little endian
        char buf[8];
        buf[0] = tmp;
        buf[1] = tmp >> 8;
        buf[2] = tmp >> 16;
        buf[3] = tmp >> 24;
        buf[4] = tmp >> 32;
        buf[5] = tmp >> 40;
        buf[6] = tmp >> 48;
        buf[7] = tmp >> 56;
        rep_.append(buf, sizeof(buf));
    };

    Slice Encode() const {
        assert(!rep_.empty());
        return rep_;
    }

private:
    std::string rep_;
};
} // namespace leveldb_clone

#endif
