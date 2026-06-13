#include "comparator.h"
#include <string>

namespace leveldb_clone {

// Put file-local classes/functions/variables in an anonymous namespace.
namespace {
class BytewiseComparatorImpl : public Comparator {
public:
  BytewiseComparatorImpl() = default;

  int Compare(const Slice &lhs, const Slice &rhs) const override {
    return lhs.compare(rhs);
  }

  // Name should not be changed
  // The comparator of the same name must behave the same
  std::string Name() const override { return "leveldb_BytewiseComparator"; }
};
} // namespace

// Why using this BytewiseComparator for users to get a pointer
// of BytewiseComparatorImpl rather than new BytewiseComparatorImpl()?
// Ans: There should only be one BytewiseComparator, no one owns it,
//      it will never be changed and it lives throguhout the process.
//      Therefore, all we need is an Static locals live in static storage,
//      initialized once on first call, persist until program exit.
//      Returning a pointer to one is safe — it outlives any caller.
const Comparator *BytewiseComparator() {
  static BytewiseComparatorImpl singleton;
  return &singleton;
}

} // namespace leveldb_clone
