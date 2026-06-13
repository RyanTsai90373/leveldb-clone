#include "string"
#include "slice.h"
#include "db/dbformat.h"

using namespace leveldb_clone;

int main() {
    InternalKey k("a", 12, kTypeValue);
    std::string r = k.Encode().ToString();
    assert(r.size() == 9);
    assert(r[0] == 'a');
    assert((uint8_t)r[1] == 0x01);  // type byte
    assert((uint8_t)r[2] == 0x0C);  // seq low byte
    for (int i = 3; i < 9; i++) assert(r[i] == 0x00);
}
