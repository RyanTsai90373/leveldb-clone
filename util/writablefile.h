#include "status.h"

#ifndef LEVELDB_CLONE_UTIL_WRITABLEFILE_H
#define LEVELDB_CLONE_UTIL_WRITABLEFILE_H

namespace leveldb_clone {

// File abstraction, interface to interact with a file
class WritableFile {
public:
    // Q: why we need this anyway? is this because we delete a constructor?
    WritableFile() = default;
    // no copy
    // Q: difference between WritableFile and WritableFile& when deleting a constructor
    WritableFile(const WritableFile&) = delete;
    WritableFile& operator=(const WritableFile&) = delete;

    virtual ~WritableFile() = default;
    virtual Status Append(const Slice& data) = 0;
    virtual Status Close() = 0;
    virtual Status Flush() = 0;
    virtual Status Sync() = 0;
};

const WritableFile* GetPosixWritableFile(const Slice& s, int fd);
} // namespace leveldb_clone

#endif
