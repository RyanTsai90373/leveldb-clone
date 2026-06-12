#include <cstdint>
#include "../include/slice.h"


namespace leveldb_clone
{
    
char* EncodeVarint32(char* dst, uint32_t v) {
    // 0x80 = 1000 0000
    while (v >= 0x80) {
        // It is impossible to just copy last 7 bits
        // we need to utilize bit operation not memcpy
        *dst = (v & 0x7F) | 0x80;
        dst++;
        v >>= 7;
    }
    *dst = v;
    dst++;
    return dst;
}

bool GetVarint32(Slice* input, uint32_t* value) {
    // avoid OR with some garbage
    *value = 0; 

    // Since we add a flag bit to every byte while encoding,
    // a 32-bit int can be encoded into up to 5 bytes.
    for (int i = 0; i < 5; i++) {
        // ran out of input before the varint ended. 
        if (input->size() == 0)
            return false;
        uint32_t byte = static_cast<uint8_t>(*input->data());

        *value |= (byte & 0x7F) << (7 * i);
        input->remove_prefix(1);
        // If MSB is 0, then it is finished
        if ((byte & 0x80) == 0)
            return true; 
    }
    return false;
}                                          

char* EncodeVarint64(char* dst, uint64_t v) {
    while (v >= 0x80) {
        *dst = (v & 0x7F) | 0x80;
        dst++;
        v >>= 7;
    }
    *dst = v;
    dst++; // return the last poistion
    return dst;
}

bool GetVarint64(Slice* input, uint64_t* value) {
    *value = 0;
    // 64 / 7 = 9...1
    for (int i = 0; i < 10; i++) {
        if (input->size() == 0)
            return false;
        uint64_t byte = static_cast<uint8_t>(*input->data());
        *value |= (byte & 0x7F) << (7 * i);
        input->remove_prefix(1);

        if ((byte & 0x80) == 0)
            return true;
    }
    return false;
}


} // namespace leveldb_clone

