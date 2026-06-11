#include <map>
#include "../include/slice.h"
#include "../include/status.h"
#include "../util/arena.h"
#include "../db/dbformat.h"

namespace leveldb_clone
{

class LookupKey;

class MemTable {
public:
    void Add(SequenceNumber seq, ValueType type, const Slice& key, const Slice& value);
    Status Get(const Slice& key, std::string* value, SequenceNumber seq);

private:
    struct comparator {
        // Descending Order
        // Return true if A should be in the front
        bool operator()(const std::string& a, const std::string& b) const {
            // compare key first
            const Slice a1 {a.data(), a.size() - 8};
            const Slice a2 {b.data(), b.size() - 8};

            int result = a1.compare(a2);
            // a1 > a2
            if (result > 0)
                return true;
            // a1 < a2
            else if (result < 0)
                return false;
            // else: same key 
            // compare sequence (ignore the first bytes (kTypeValue, kTypeDeletion))
            for (size_t i = a.size() - 1; i >= a.size() - 7; i--) {
                if (a[i] > b[i]) 
                    return true;
                else if (a[i] < b[i])
                    return false;
            }
            // no way both key and sequence is the same 
            return false;
        }
    };

    Arena arena_;
    std::map<std::string, char*, comparator> table_;
};
    
} // namespace leveldb_clone 
