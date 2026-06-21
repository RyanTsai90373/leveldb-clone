#include <cassert>
#include "../db/log_writer.h"
#include "../db/log_reader.h"
#include "../include/env.h"
#include "status.h"



using namespace leveldb_clone;
using namespace log;

int main () {
    const Slice fname("WAL.log");
    WritableFile* wfile;
    if (!GetWritableFile(fname, &wfile).IsOk())
        printf("Write Log Error");

    LogWriter writer(wfile);
    const Slice data("Hello World!");
    writer.Append(data);
    delete wfile;

    SequentialFile* sqfile;
    if(!GetSequentialFile(fname, &sqfile).IsOk())
        printf("Reader Error");

    Reader reader(sqfile);
    Slice record;
    std::string str;
    reader.ReadRecord(&record, &str);
    assert(record == data);
    delete sqfile;
}
    




