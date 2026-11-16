// mem_file.hpp — IFile de solo lectura sobre un buffer en memoria (para tests).
#pragma once

#include <cstring>

#include "ereader/ifile.hpp"

namespace ereader {
namespace native {

class MemFile : public IFile {
public:
    MemFile(const uint8_t* data, uint64_t size) : data_(data), size_(size) {}

    Result read(void* out, size_t len, size_t& readBytes) override {
        uint64_t avail = (pos_ < size_) ? (size_ - pos_) : 0;
        size_t n = static_cast<size_t>(len < avail ? len : avail);
        std::memcpy(out, data_ + pos_, n);
        pos_ += n;
        readBytes = n;
        return Result::Ok;
    }
    Result write(const void*, size_t) override { return Result::ErrUnsupported; }
    Result seek(int64_t offset, Seek whence) override {
        int64_t base = (whence == Seek::Set) ? 0
                      : (whence == Seek::Cur) ? static_cast<int64_t>(pos_)
                      : static_cast<int64_t>(size_);
        int64_t np = base + offset;
        if (np < 0) np = 0;
        if (static_cast<uint64_t>(np) > size_) np = static_cast<int64_t>(size_);
        pos_ = static_cast<uint64_t>(np);
        return Result::Ok;
    }
    uint64_t tell() const override { return pos_; }
    uint64_t size() const override { return size_; }
    Result flush() override { return Result::Ok; }
    void close() override {}

private:
    const uint8_t* data_;
    uint64_t size_;
    uint64_t pos_ = 0;
};

}  // namespace native
}  // namespace ereader
