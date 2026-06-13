// status.h is an wrapper of an enum, provides status code and msg
#include "slice.h"

#ifndef LEVELDB_CLONE_INCLUDE_STATUS_H
#define LEVELDB_CLONE_INCLUDE_STATUS_H

namespace leveldb_clone {

class Status {
public:
    // Encapsulate construction so that users can only construct a Status with valid Code
    // Make sure the functions are static since the constructor is private
    static Status Ok() { return Status(Code::kOk, ""); }
    static Status NotFound(const Slice &s) { return Status(Code::kNotFound, s); }
    static Status Corruption(const Slice &s) { return Status(Code::kCorruption, s); }
    static Status NotSupported(const Slice &s) { return Status(Code::kNotSupported, s); }
    static Status InvalidArg(const Slice &s) { return Status(Code::kInvalidArgument, s); }
    static Status IOError(const Slice &s) { return Status(Code::kIOError, s); }

    // Every member function should be const if it does not modify member variables
    bool IsOk() const { return code_ == Code::kOk; }
    bool IsNotFound() const { return code_ == Code::kNotFound; }
    bool IsCorruption() const { return code_ == Code::kCorruption; }
    bool IsNotSupported() const { return code_ == Code::kNotSupported; }
    bool IsInvalidArg() const { return code_ == Code::kInvalidArgument; }
    bool IsIOError() const { return code_ == Code::kIOError; }

private:
    enum Code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5
    };
    Code code_;
    std::string msg_;
    // Since the argument is a "const", ToString() must also be a const function 
    // so that the compiler knows msg will not be modified.
    Status(Code code, const Slice& msg) : code_(code), msg_(msg.ToString()) {}
};

}  // namespace leveldb_clone

#endif