// stdio_fs.cpp — Implementación de StdioFile/StdioFileSystem.
#include "platform_native/stdio_fs.hpp"

#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace ereader {
namespace native {

Result StdioFile::read(void* out, size_t len, size_t& readBytes) {
    if (!fp_) return Result::ErrIO;
    readBytes = std::fread(out, 1, len, fp_);
    if (readBytes < len && std::ferror(fp_)) return Result::ErrIO;
    return Result::Ok;
}

Result StdioFile::write(const void* data, size_t len) {
    if (!fp_) return Result::ErrIO;
    size_t w = std::fwrite(data, 1, len, fp_);
    return w == len ? Result::Ok : Result::ErrIO;
}

Result StdioFile::seek(int64_t offset, Seek whence) {
    if (!fp_) return Result::ErrIO;
    int w = (whence == Seek::Set) ? SEEK_SET : (whence == Seek::Cur) ? SEEK_CUR : SEEK_END;
    return std::fseek(fp_, static_cast<long>(offset), w) == 0 ? Result::Ok : Result::ErrIO;
}

uint64_t StdioFile::tell() const {
    if (!fp_) return 0;
    long p = std::ftell(fp_);
    return p < 0 ? 0 : static_cast<uint64_t>(p);
}

Result StdioFile::flush() {
    if (!fp_) return Result::ErrIO;
    return std::fflush(fp_) == 0 ? Result::Ok : Result::ErrIO;
}

void StdioFile::close() {
    if (fp_) {
        std::fclose(fp_);
        fp_ = nullptr;
    }
}

StdioFileSystem::StdioFileSystem(const char* root) {
    std::strncpy(root_, root ? root : "", sizeof(root_) - 1);
    root_[sizeof(root_) - 1] = '\0';
}

bool StdioFileSystem::resolve(const char* path, char* buf, size_t cap) const {
    size_t rl = std::strlen(root_);
    size_t pl = std::strlen(path);
    if (rl + pl + 1 > cap) return false;
    std::memcpy(buf, root_, rl);
    std::memcpy(buf + rl, path, pl + 1);
    return true;
}

Result StdioFileSystem::open(const char* path, OpenMode mode, IFile*& out) {
    char full[512];
    if (!resolve(path, full, sizeof(full))) return Result::ErrInvalidArg;
    const char* m = (mode == OpenMode::Read) ? "rb"
                   : (mode == OpenMode::Update) ? "r+b"
                   : "w+b";
    FILE* fp = std::fopen(full, m);
    if (!fp) return Result::ErrNotFound;
    uint64_t sz = 0;
    if (mode == OpenMode::Read || mode == OpenMode::Update) {
        std::fseek(fp, 0, SEEK_END);
        long e = std::ftell(fp);
        sz = e < 0 ? 0 : static_cast<uint64_t>(e);
        std::fseek(fp, 0, SEEK_SET);
    }
    out = new StdioFile(fp, sz);
    return Result::Ok;
}

bool StdioFileSystem::exists(const char* path) {
    char full[512];
    if (!resolve(path, full, sizeof(full))) return false;
    struct stat st;
    return ::stat(full, &st) == 0;
}

Result StdioFileSystem::remove(const char* path) {
    char full[512];
    if (!resolve(path, full, sizeof(full))) return Result::ErrInvalidArg;
    return std::remove(full) == 0 ? Result::Ok : Result::ErrIO;
}

Result StdioFileSystem::rename(const char* from, const char* to) {
    char f[512], t[512];
    if (!resolve(from, f, sizeof(f)) || !resolve(to, t, sizeof(t)))
        return Result::ErrInvalidArg;
    return std::rename(f, t) == 0 ? Result::Ok : Result::ErrIO;
}

Result StdioFileSystem::mkdir(const char* path) {
    char full[512];
    if (!resolve(path, full, sizeof(full))) return Result::ErrInvalidArg;
    if (::mkdir(full, 0755) == 0) return Result::Ok;
    // No es error si ya existe.
    struct stat st;
    if (::stat(full, &st) == 0) return Result::Ok;
    return Result::ErrIO;
}

Result StdioFileSystem::listDir(const char* path, DirCallback cb, void* user) {
    char full[512];
    if (!resolve(path, full, sizeof(full))) return Result::ErrInvalidArg;
    DIR* dir = ::opendir(full);
    if (!dir) return Result::ErrNotFound;
    struct dirent* ent;
    Result res = Result::Ok;
    while ((ent = ::readdir(dir)) != nullptr) {
        if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0)
            continue;
        char child[700];
        std::snprintf(child, sizeof(child), "%s/%s", full, ent->d_name);
        struct stat st;
        uint64_t sz = 0;
        bool isDir = false;
        if (::stat(child, &st) == 0) {
            sz = static_cast<uint64_t>(st.st_size);
            isDir = S_ISDIR(st.st_mode);
        }
        DirEntry e{ent->d_name, sz, isDir};
        if (!cb(e, user)) break;
    }
    ::closedir(dir);
    return res;
}

uint64_t StdioFileSystem::freeBytes() {
    // En el PC se reporta un valor grande y fijo; no es relevante para los tests.
    return static_cast<uint64_t>(1) << 40;  // 1 TiB simbólico.
}

}  // namespace native
}  // namespace ereader
