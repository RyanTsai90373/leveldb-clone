#include <string>
#include "db.h"
#include "status.h"
#include "slice.h"
#include "../db/memtable.h"
#include "../db/dbformat.h"
#include "../util/arena.h"

#ifndef LEVELDB_CLONE_INCLUDE_DB_IMPL_H
#define LEVELDB_CLONE_INCLUDE_DB_IMPL_H

namespace leveldb_clone {

class DBImpl : public DB {
public:
    DBImpl() = default;
    DBImpl(const Slice& name) : name_(name.ToString()) {}
    Status Put(const Slice& key, const Slice& value) override;
    Status Get(const Slice& key, std::string* value) override;
    Status Delete(const Slice& key) override;
private:
    // tmp
    SequenceNumber seq_ = 0;
    std::string name_;
    MemTable table_; 
};

}  // namespace leveldb_clone

#endif
