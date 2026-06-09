#include <string>
#include "slice.h"

#ifndef LEVELDB_CLONE_INCLUDE_COMPARATOR_H
#define LEVELDB_CLONE_INCLUDE_COMPARATOR_H

namespace leveldb_clone {

class Comparator {
public:
    // Virtual destructor must be defined!
    virtual ~Comparator() = default;

    virtual int Compare (const Slice& lhs, const Slice& rhs) const = 0;

    virtual std::string Name() const = 0;

};

// Forward declaration for every file includes the comparator.h
// since its implementation is in .cc no one can use it.
const Comparator* BytewiseComparator();

}  // namespace leveldb_clone

#endif