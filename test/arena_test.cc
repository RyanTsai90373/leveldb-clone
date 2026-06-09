#include <cassert>
#include <string>
#include "../util/arena.h"
#include "../include/slice.h"
#include "../include/status.h"

using namespace leveldb_clone;

int main () {
   Arena a; 
   a.Allocate(5);
   a.Allocate(5);
   assert(a.MemoryUsage() == 4096);

   Arena b;
   b.Allocate(5000);
   assert(b.MemoryUsage() == 5000);

   Arena c;
   c.Allocate(4095);
   c.Allocate(10);
   assert(c.MemoryUsage() == (2 * 4096));
}
