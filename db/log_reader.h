#include "../util/coding.h"
#include "env.h"
#include "slice.h"
#include "status.h"
#include "logformat.h"
#include <fcntl.h>
#include <string>
// Given a binary log file, needs to build a memtable out of it
// Log Layout [CRC32 4bytes | Len 2bytes | Type 1bytes | data]
// memtable::Add(seq, type, key, value)
// Value Layout [KeySize | KeyBytes | Tag(Seq+Type) | ValueSize | Value]
// 1. checksum, if the data is corrupted, then drop it
// 2. both size are variant32, decode it and get the key value (now memtable is not implemented this way)
// 3. decodefixed64(seq) get the SequenceNumber, and get the type
// 3. if vtype=full -> add to memtable
// 4. if vtype=first -> save it to buffer, keep reading blocks
// 5. till type=last -> add to memtable

constexpr size_t kBlockSize = 32768;
namespace leveldb_clone {

namespace log {

// Only chekc the validation of data and parse data
// One Key Value pair a time
class Reader {
public:
	Reader(SequentialFile* seqfile) : seqfile_(seqfile), block_(0), offset_(0) {}
	// Composition over inheritance
	class Report {};
	// virtual Status OnError();

	// Return type:
	// cannot return slice since it will be dangling pointer
	// let the users pass in char* , modify inplace, and return size
	bool ReadRecord(Slice* record, std::string* scatch);
	RecordType ReadPhysicalRecord(std::string** scatch);

private:
	SequentialFile* seqfile_; 
	size_t block_;
	size_t offset_;
	char buf_[kBlockSize]; // load a block at once, not a record
};

} // namespace log
} // namespace leveldb_clone
