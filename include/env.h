#include <unistd.h>
#include "status.h"

#ifndef LEVELDB_CLONE_INCLUDE_ENV_H
#define LEVELDB_CLONE_INCLUDE_ENV_H

namespace leveldb_clone {

// A file abstraction for reading sequentially through a file
class SequentialFile {
public:
    SequentialFile() = default;
	virtual ~SequentialFile() = default;

	SequentialFile(SequentialFile&) = delete;
	SequentialFile& operator=(const SequentialFile&) = delete;

	virtual Status Skip(int64_t n) = 0;
	virtual Status Read(char* buf, size_t n, size_t* actual_read) = 0;
	
private:
};

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

Status GetWritableFile(const Slice& s, WritableFile**);
Status GetSequentialFile(const Slice& s, SequentialFile**);


} // namespace leveldb_clone

#endif
