// Arena isa memory 
#include <atomic>
#include <vector>
#include <stddef.h>

#ifndef LEVELDB_CLONE_UTIL_ARENA_H
#define LEVELDB_CLONE_UTIL_ARENA_H

namespace leveldb_clone {

class Arena {                                                   
public:                                                                
    Arena() : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0) {};                                                              
    ~Arena();                                                                 
    Arena(const Arena&) = delete;                                             
    Arena& operator=(const Arena&) = delete;                                    
   
    char* Allocate(size_t bytes);   
    // TODO: Understand what is atomic and why we have to use it
    size_t MemoryUsage() const { 
        return memory_usage_.load(std::memory_order_relaxed); 
    }
                                                                  
private:                                                                     
    char* alloc_ptr_;                                                     
    size_t alloc_bytes_remaining_;
    std::vector<char*> blocks_;                                                 
    std::atomic<size_t> memory_usage_;

};

}  // namespace leveldb_clone

#endif