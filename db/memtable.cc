#include "memtable.h"
#include "dbformat.h"
#include "slice.h"
#include "status.h"

namespace leveldb_clone {

void MemTable::Add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value) {
    // [size of all][Key + Seq(8bytes)][Value]
    InternalKey k {key, seq, type};
    Slice inkey = k.Encode();
    
    // TODO: variant version, first bit is to represent whether to continue reading 
    // Currently using fixed one byte header, suppose it will not take more than 7 bytes
    // to represent Internalkey + value
    uint8_t kv_size = inkey.size() + value.size();
    int total_bytes = 1 + kv_size;
    char* data = arena_.Allocate(total_bytes);

    // Move data to arena
    char* p = data;
    memcpy(p, &kv_size, sizeof(kv_size));
    p += sizeof(kv_size);
    memcpy(p, inkey.data(), inkey.size());
    p += inkey.size();
    memcpy(p, value.data(), value.size());

    table_[inkey.ToString()] = data;
}

// TODO: Change it to LookupKey
Status MemTable::Get(const Slice& key, std::string* value, SequenceNumber seq) {
    // Exact match
    InternalKey inkey {key, seq, kTypeValue};
    std::string key_str = inkey.Encode().ToString();
    // lower_bound: return the first element that (*it, k) is false
    // We pass in a descending comparator, which means lower_bound
    // will find the value that is <= key_str
    auto it = table_.lower_bound(key_str);


    // Find a match, can either be a value or a deletion 
    if (it != table_.end()) {
        // Check if the key is the same
        Slice stored_user_key (it->first.data(), it->first.size() - 8);
        if (key != stored_user_key)
            return Status::NotFound("Wrong Key"); 

        // The eighth bit of first byte decides whether it is a put or deleteion
        if (static_cast<uint8_t>(it->first[key.size()]) == 0)
            return Status::NotFound("Key Deleted");

        // Data in the Arena
        const char* p = it->second;
        uint8_t msg_size = static_cast<uint8_t>(*p);
        // 2. 1bytes header + 8bytes internalkey
        p += 1 + key_str.size();
        // 3. after that is the message
        *value = {p, msg_size - key_str.size()};
        return Status::Ok();
    }
    return Status::NotFound("Wrong key");
}

}
