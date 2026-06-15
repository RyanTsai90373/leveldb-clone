#include <cstdint>
#include "slice.h"

#ifndef LEVELDB_CLONE_UTIL_CODING_H
#define LEVELDB_CLONE_UTIL_CODING_H

namespace leveldb_clone
{

char* EncodeVarint32(char* dst, uint32_t v);
bool GetVarint32(Slice* input, uint32_t* value);
char* EncodeVarint64(char* dst, uint64_t v);
bool GetVarint64(Slice* input, uint64_t* value);


inline void EncodeFixed16(char* dst, uint32_t value) {
    for (int i = 0; i < 2; i++) {
        uint8_t byte = static_cast<uint8_t>(value);
        *dst = byte; 
        dst++;
        value >>= 8;
    }
}
inline void EncodeFixed32(char* dst, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        uint8_t byte = static_cast<uint8_t>(value);
        *dst = byte; 
        dst++;
        value >>= 8;
    }
}
inline void EncodeFixed64(char* dst, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        uint8_t byte = static_cast<uint8_t>(value);
        *dst = byte; 
        dst++;
        value >>= 8;
    }
}
inline uint32_t DecodeFixed32(const char* ptr) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        // Use uint8_t to avoid signed -> unsigned conversion
        // which will fill the higher bit with its MSB 
        uint32_t byte = static_cast<uint8_t>(ptr[i]);
        value |= (byte << (8 * i));
    }    
    return value;
}
inline uint64_t DecodeFixed64(const char* ptr) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        uint64_t byte = static_cast<uint8_t>(ptr[i]);
        value |= (byte << (8 * i));
    }    
    return value;
}

} // namespace leveldb_clone
#endif
