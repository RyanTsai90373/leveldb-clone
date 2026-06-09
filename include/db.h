// The Core of LevelDB, provides API to set and get data

#include "status.h"
#include "slice.h"
#include <string>

#ifndef LEVELDB_CLONE_DB_H
#define LEVELDB_CLONE_DB_H

namespace leveldb_clone {

class DB {
public:
    DB() = default;
    virtual ~DB() = default; 

    DB(const DB&) = delete;                                                                
    DB& operator=(const DB&) = delete;    

    static Status Open(const Slice& name, DB** dbptr);
    virtual Status Put(const Slice& key, const Slice& value) = 0;
    virtual Status Delete(const Slice& key) = 0;
    virtual Status Get(const Slice& key, std::string* value) = 0;
};

}  // namespace leveldb_clone

#endif