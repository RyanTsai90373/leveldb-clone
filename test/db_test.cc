#include <cassert>
#include <string>
#include "db.h"
#include "slice.h"
#include "status.h"
#include "../db/dbformat.h"

using namespace leveldb_clone;

int main () {
    DB* mydb = nullptr; 
    Status st = DB::Open(Slice("mydb"), &mydb);
    assert(st.IsOk());

    std::string s;
    mydb->Put("abc", "1");
    mydb->Get("abc", &s);
    assert(s == "1");

    mydb->Put("a", "test2");
    mydb->Get("a", &s);
    assert(s == "test2");

    mydb->Put("apple", "good");
    mydb->Get("apple", &s);
    assert(s == "good");

    assert(mydb->Get("banana", &s).IsNotFound());


    mydb->Delete("a");
    assert(mydb->Get("a", &s).IsNotFound());

    delete mydb;
}
