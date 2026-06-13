#include "../include/db_impl.h"

namespace leveldb_clone {

Status DB::Open(const Slice& name, DB** dbptr) {
    *dbptr = new DBImpl(name);
    return Status::Ok();
}

Status DBImpl::Put(const Slice& key, const Slice& value) {
    table_.Add(seq_, kTypeValue, key, value);
    seq_++;
    return Status::Ok();
}
Status DBImpl::Get(const Slice& key, std::string* value) {
    return table_.Get(key, value, seq_);
}
Status DBImpl::Delete(const Slice& key) {
    table_.Add(seq_, kTypeDeletion, key, "");
    seq_++;
    return Status::Ok();
}

}  // namespace leveldb_clone


