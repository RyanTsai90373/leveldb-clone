#include "memtable.h"
#include "dbformat.h"
#include "slice.h"
#include "status.h"
#include "../util/coding.h"

namespace leveldb_clone {

void MemTable::Add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value) {
    // Format of an entry is concatenation of:
    //  key_size     : varint32 of internal_key.size()
    //  key bytes    : char[internal_key.size()]
    //  tag          : uint64((sequence << 8) | type)
    //  value_size   : varint32 of value.size()
    //  value bytes  : char[value.size()]
    InternalKey k{key, seq, type};
    Slice inkey = k.Encode();

    size_t max_encoded = 5 + inkey.size() + 5 + value.size();
    char* data = arena_.Allocate(max_encoded);

    char* p = data;
    p = EncodeVarint32(p, inkey.size());                      // move 1~5 bytes forward
    memcpy(p, inkey.data(), inkey.size()); p += inkey.size(); // actual data size (ex. 1000 bytes)
    p = EncodeVarint32(p, value.size()); 
    memcpy(p, value.data(), value.size()); p += value.size();

    table_[inkey.ToString()] = data;
}

// TODO: Change it to LookupKey
Status MemTable::Get(const Slice& key, std::string* value, SequenceNumber seq) {
    // Exact match
    InternalKey inkey {key, seq, kTypeValue};
    std::string inkey_str = inkey.Encode().ToString();
    // lower_bound: return the first element that (*it, k) is false
    // We pass in a descending comparator, which means lower_bound
    // will find the value that is <= inkey_str
    auto it = table_.lower_bound(inkey_str);


    // Find a match, can either be a value or a deletion 
    if (it != table_.end()) {
        // Check if the key is the same
        Slice stored_user_key (it->first.data(), it->first.size() - 8);
        if (key != stored_user_key)
            return Status::NotFound("Wrong Key"); 

        // The eighth bit of first byte decides whether it is a put or deleteion
        if (static_cast<uint8_t>(it->first[key.size()]) == 0)
            return Status::NotFound("Key Deleted");

        Slice cursor(it->second, SIZE_MAX);   
                                                                                                                    
        uint32_t key_size, value_size;                                   
        GetVarint32(&cursor, &key_size);                                                                            
        cursor.remove_prefix(key_size);            // skip past the internal key
        GetVarint32(&cursor, &value_size);                                                                          
        *value = std::string(cursor.data(), value_size);                                                          
        return Status::Ok(); 
    }
    return Status::NotFound("Wrong key");
}

}
