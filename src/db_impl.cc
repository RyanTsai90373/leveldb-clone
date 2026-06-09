#include "db_impl.h"

namespace leveldb_clone {

Status DB::Open(const Slice& name, DB** dbptr) {
    *dbptr = new DBImpl(name);
    return Status::Ok();
}

Status DBImpl::Put(const Slice& key, const Slice& value) {
    table_[key.ToString()] = value.ToString();
    return Status::Ok();
}
Status DBImpl::Get(const Slice& key, std::string* value) {
    // operator [] will create a key if the key doesn't exist so we must use .find()
    auto it = table_.find(key.ToString());
    if (it == table_.end())
        return Status::NotFound("wrong key");
    *value = it->second;
    return Status::Ok();
}
Status DBImpl::Delete(const Slice& key) {
    table_.erase(key.ToString());
    return Status::Ok();
}

}  // namespace leveldb_clone


