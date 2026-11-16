// stdio_fs.hpp — Implementación de IFile/IFileSystem sobre stdio (entorno PC).
//
// Solo para `native`: tests y CLI de paginación. NO se compila en el firmware.
#pragma once

#include <cstdio>

#include "ereader/ifile.hpp"
#include "ereader/ifilesystem.hpp"

namespace ereader {
namespace native {

class StdioFile : public IFile {
public:
    explicit StdioFile(FILE* fp, uint64_t size) : fp_(fp), size_(size) {}
    ~StdioFile() override { close(); }

    Result read(void* out, size_t len, size_t& readBytes) override;
    Result write(const void* data, size_t len) override;
    Result seek(int64_t offset, Seek whence) override;
    uint64_t tell() const override;
    uint64_t size() const override { return size_; }
    Result flush() override;
    void close() override;

private:
    FILE* fp_;
    uint64_t size_;
};

class StdioFileSystem : public IFileSystem {
public:
    // `root` es un prefijo opcional que se antepone a todas las rutas
    // (para sandboxear los tests en un directorio temporal).
    explicit StdioFileSystem(const char* root = "");

    Result open(const char* path, OpenMode mode, IFile*& out) override;
    bool exists(const char* path) override;
    Result remove(const char* path) override;
    Result rename(const char* from, const char* to) override;
    Result mkdir(const char* path) override;
    Result listDir(const char* path, DirCallback cb, void* user) override;
    uint64_t freeBytes() override;

private:
    // Compone root_ + path en `buf`. Devuelve false si no cabe.
    bool resolve(const char* path, char* buf, size_t cap) const;
    char root_[256];
};

}  // namespace native
}  // namespace ereader
