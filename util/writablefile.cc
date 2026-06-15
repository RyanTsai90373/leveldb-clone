#include "writablefile.h"

#include <unistd.h>

#include <cstdint>
#include <string>

#include "status.h"

namespace leveldb_clone {

const uint64_t kWritableFileBufferSize = 65536;

// write in batch to reduce call of write() & fsync()
class PosixWritableFile : public WritableFile {
   public:
    PosixWritableFile(const std::string& filename, int fd) : fd_(fd), filename_(filename), pos_(0) {}

    ~PosixWritableFile() { Close(); }

    // clear buf_
    Status Flush() override {
        // Maybe Flush should call Sync? Don't know how to combine write and fsync
        if (::write(fd_, buf_, pos_) < 0) 
            return Status::IOError("Flush Fail");
        pos_ = 0;
        return Status::Ok();
    }

    Status Sync() override { 
        Status s = Flush();
        if (!s.IsOk()) 
            return Status::IOError("Flush Fail");
        // data in the kernel space
        if (::fsync(fd_) < 0)
            return Status::IOError("Fail to write into disk");
        return Status::Ok(); 
    }

    Status Append(const Slice& data) override {
        size_t data_size = data.size();

        // If there are enough spaces, just append data into buffer
        if (data_size <= (kWritableFileBufferSize - pos_)) {
            memcpy(&buf_[pos_], data.data(), data.size());
            pos_ += data_size;
            return Status::Ok();
        }
        Flush();

        // if the new data can't fit into buffer
        if (data_size > kWritableFileBufferSize) {
            if (::write(fd_, data.data(), data.size()) < 0)
                return Status::IOError("Append Fail!");
        }   
        else {
            memcpy(&buf_[pos_], data.data(), data.size());
            pos_ = data_size;
        }

        return Status::Ok();
    }

    Status Close() override {
        // clear buf
        Flush();
        if (close(fd_) < 0) 
            return Status::IOError("Fail to clsoe a file");
        return Status::Ok();
    }

   private:
    int fd_;
    char buf_[kWritableFileBufferSize];
    std::string filename_;
    // std::string dirname_;
    size_t pos_;
};

const WritableFile* GetPosixWritableFile(const Slice& filename, int fd) {
	static WritableFile* f = new PosixWritableFile{std::string{filename.data(), filename.size()}, fd}; 
	return f;
}

}  // namespace leveldb_clone
