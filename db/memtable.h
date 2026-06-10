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
    Arena arena_;
    std::map<std::string, char*> table_;
};
    
} // namespace leveldb_clone 
