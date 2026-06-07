#include <map>
#include <string>
#include "db.h"
#include "status.h"
#include "slice.h"

class DBImpl : public DB {
public:
    DBImpl() = default;
    DBImpl(const Slice& name) : name_(name.ToString()) {}
    Status Put(const Slice& key, const Slice& value) override; 
    Status Get(const Slice& key, std::string* value) override;
    Status Delete(const Slice& key) override;
private:
    std::string name_;
    std::map<std::string, std::string> table_;
};


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


