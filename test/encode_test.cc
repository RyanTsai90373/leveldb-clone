#include <cassert>
#include "slice.h"
#include "util/coding.h"

using namespace leveldb_clone;

int main() {

    uint32_t values[] = {                                                                      
      0,           // smallest                                                               
      1,           // 1-byte boundary                                                        
      127,         // last 1-byte value                                                      
      128,         // first 2-byte value                                                     
      16383,       // last 2-byte value                                                      
      16384,       // first 3-byte value                                       
      12345678,    // your current case                                                      
      (1u << 28) - 1,  // last 4-byte value                                                
      (1u << 28),      // first 5-byte value                                   
      0xFFFFFFFF,      // largest uint32                                                     
    };  

    for (auto v: values) {
        char dst[5];
        char* end = EncodeVarint32(dst, v);
        size_t len = end - dst;

        Slice s {dst, len};
        uint32_t ans;
        GetVarint32(&s, &ans);
        assert(ans == v);
    }

    uint64_t n2 = 123456789;
    char dst2[10];
    EncodeFixed64(dst2, n2);
    uint64_t ans2 = DecodeFixed64(dst2);
    assert(ans2 == n2);


}
