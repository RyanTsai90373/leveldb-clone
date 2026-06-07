// Slice in leveldb is basically its own version of std::str_view

#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>

// To avoid defining Slice in a translation unit twice
// These guards should be in every .h file
#ifndef LEVELDB_CLONE_INCLUDE_SLICE_H
#define LEVELDB_CLONE_INCLUDE_SLICE_H

class Slice {
public:
    // ===== Constructor =====
    // The default constrcutor put "" instead of nullptr, which ensures that
    // the data will never be a nullptr, and we will not have to check 
    // if (data_ == nullptr) before using it
    Slice() : data_(""), size_(0) {}
    // Standard
    Slice(const char* d, size_t n) : data_(d), size_(n) {}
     
    Slice(const char* d) : data_(d), size_(strlen(d)) {}
    
    Slice(const std::string &s) : data_(s.data()), size_(s.size()) {}

    // ===== API =====
    // 1. It should be able to get the value it is pointing to
    size_t size() const { return size_; }
    const char* data() const { return data_; }

    // Setter in this case is redundant because C++ automatically 
    // generate copy assignment operator and copy constructor
    /*  
        void SetData(const char* d, size_t n) { data_ = d; size_ = n; } 
        void SetData(const char* d) { data_ = d; size_ = strlen(d); } 
        void SetData(const std::string &s) {data_ = s.data(); size_ = s.size(); }
    */

    // ===== Comparison =====
    int compare(const Slice& rhs) const { 
        const size_t n = std::min(size_, rhs.size_);
        int r = std::memcmp(data_, rhs.data_, n);
        if (r == 0) {
            if (size_ == rhs.size_)
                return 0;
            else if (size_ < rhs.size_)
                return -1;
            else 
                return 1; 
        }
        return r;
    }
    bool operator==(const Slice& rhs) const {
        return ((size_ == rhs.size_) && 
                (std::memcmp(data_, rhs.data_, size_) == 0)); 
    }
    bool operator!=(const Slice& rhs) const { return !(*this == rhs); }

    // String can be constructed from const char*, it reads forward until hitting \0
    // However, if we didn't specify the size there might be some problems.
    // 1. The const char* we are passing in does not have a \0 at the end of the char array
    // 2. The string itself contains \0, thus we might stop before going through the whole string
    std::string ToString() const { return std::string(data_, size_); }

private:
    const char* data_;
    size_t size_;
};

#endif