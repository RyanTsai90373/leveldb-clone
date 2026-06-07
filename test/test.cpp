#include <cassert>
#include <string>
#include "../include/db.h"
#include "../include/slice.h"
#include "../include/status.h"

int main () {
    DB* mydb = nullptr; 
    Status st = DB::Open(Slice("mydb"), &mydb);
    assert(st.IsOk());

    std::string s;
    mydb->Put("a", "1");
    mydb->Get("a", &s);
    assert(s == "1");

    mydb->Put("a", "test2");
    mydb->Get("a", &s);
    assert(s == "test2");

    mydb->Delete("a");
    assert(mydb->Get("a", &s).IsNotFound());
}