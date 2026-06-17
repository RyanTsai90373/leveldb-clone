#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <string>
#include "status.h"
#include "env.h"

namespace leveldb_clone {

const uint64_t kWritableFileBufferSize = 65536;

// write in batch to reduce call of write() & fsync()
class PosixWritableFile : public WritableFile {
   public:
    PosixWritableFile(int fd, const std::string& filename) : fd_(fd), filename_(filename), pos_(0) {}

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
    size_t pos_;
};


class PosixSequentialFile : public SequentialFile {
public:
	PosixSequentialFile(int fd, const Slice& fname) : fd_(fd), fname_(fname.data(), fname.size()) {}
    ~PosixSequentialFile() { ::close(fd_);}

    Status Skip(int64_t n) override { 
        if(lseek(fd_, n, SEEK_CUR) < 0)
            return Status::IOError("Skip Fail");
        return Status::Ok();
    }
    // TODO: LevelDB implement in a while loop
    Status Read(char* buf, size_t n, size_t* actual_read) override {
        // ::read manage the current potision
        ssize_t result = ::read(fd_, buf, n);
        // only set actual_read when success
        if (result < 0)
            return Status::IOError("IOError");
        *actual_read = result;
        return Status::Ok();
    }
private:
	int fd_;
	std::string fname_;
};

Status GetWritableFile(const Slice& s, WritableFile** wrfile) {
    int fd = ::open(s.ToString().c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
    if (fd < 0)
        return Status::IOError("Fail to open file");
    *wrfile = new PosixWritableFile(fd, std::string{s.data(), s.size()});
    return Status::Ok();
}

Status GetSequentialFile(const Slice& s, SequentialFile** sqfile) {
    int fd = ::open(s.ToString().c_str(), O_RDONLY);
    if (fd < 0)
        return Status::IOError("Fail to open file");
    *sqfile = new PosixSequentialFile(fd, std::string{s.data(), s.size()});
    return Status::Ok();
}



}  // namespace leveldb_clone
