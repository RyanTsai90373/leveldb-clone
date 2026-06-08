#include <algorithm>
#include "arena.h"

const size_t kDefaultSize = 4096;

Arena::~Arena() {
    for (auto ptr : blocks_)
        delete[] ptr;
}

// The size of char* in side the vector
char* Arena::Allocate(size_t bytes) {
    if (alloc_bytes_remaining_ < bytes) {
        // set a default size, but should allow larger size
        size_t size_ = std::max(bytes, kDefaultSize);
        alloc_ptr_ = new char[size_];
        // blocks stores the start of every preallocate space
        blocks_.push_back(alloc_ptr_);
        alloc_bytes_remaining_ = size_;
        memory_usage_ += size_;
    }

    char* ptr = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;

    return ptr;
}