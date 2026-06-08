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