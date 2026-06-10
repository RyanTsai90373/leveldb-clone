#include "memtable.h"
#include "dbformat.h"
#include "../include/slice.h"
#include "../include/status.h"

namespace leveldb_clone {

void MemTable::Add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value) {
    // [size of all][Key + Seq(8bytes)][Value]
    // 1. Compute Internal Key
    InternalKey inkey {key, seq, type};
    std::string inkey_str = inkey.Encode().ToString();
    
    // 2. Concatenate with value
    std::string kv = inkey_str + value.ToString();
    // TODO: variant version, first bit is to represent whether to continue reading 
    // Currently using fixed one byte header, suppose it will not take more than 7 bytes
    // to represent Internalkey + value
    uint8_t header_size = kv.size();
    int total_bytes = 1 + kv.size();
    char* data = arena_.Allocate(total_bytes);
    
    // 3. Put a header of how many bytes needed
    std::string s;
    s = static_cast<char>(header_size) + kv;
    memcpy(data, s.data(), s.size());

    table_[inkey_str] = data;
    // Seq++ should be performed by DB
    // seq_++;
}

// TODO: Change it to LookupKey
Status MemTable::Get(const Slice& key, std::string* value, SequenceNumber seq) {
    bool deleted = false;
    // very inefficient, needs to adjust 
    for (int i = seq; i >= 0; --i) {
        // Exact match
        InternalKey ikey {key, i, kTypeValue};
        std::string key_str = ikey.Encode().ToString();
        auto it = table_.find(key_str);
        // No exact match, maybe a deletion
        if (it == table_.end()) {
            ikey = {key, i, kTypeDeletion};
            key_str = ikey.Encode().ToString();
            it = table_.find(key_str);
            if (it != table_.end())
                deleted = true;
        }

        // Find a match, can be a value, or a deletion 
        if (it != table_.end()) {
            if (deleted) return Status::NotFound("Deleted");

            const char* p = it->second;
            uint8_t msg_size = static_cast<uint8_t>(*p);
            // 2. 1bytes header + 8bytes internalkey
            p += 1 + key_str.size();
            // 3. after that is the message
            *value = {p, msg_size - key_str.size()};
            return Status::Ok();
        }
    }
    return Status::NotFound("wrong key");
}

}