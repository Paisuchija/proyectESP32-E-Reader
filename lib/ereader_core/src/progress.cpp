// progress.cpp — Implementación del progreso atómico.
#include "ereader/progress.hpp"

#include <cstring>

#include "ereader/byteio.hpp"
#include "ereader/config.hpp"
#include "ereader/crc32.hpp"

namespace ereader {

namespace {
constexpr size_t kRecSize = 44;
constexpr uint16_t kVersion = 1;

// Compone <path><suffix> en buf.
bool makePath(const char* path, const char* suffix, char* buf, size_t cap) {
    size_t pl = std::strlen(path), sl = std::strlen(suffix);
    if (pl + sl + 1 > cap) return false;
    std::memcpy(buf, path, pl);
    std::memcpy(buf + pl, suffix, sl + 1);
    return true;
}

void encode(const BookId& id, const Progress& p, uint8_t* rec) {
    std::memset(rec, 0, kRecSize);
    std::memcpy(rec, "ERPG", 4);
    bio::putU16(rec + 4, kVersion);
    bio::putU16(rec + 6, 0);
    bio::putU32(rec + 8, id.crc);
    bio::putU64(rec + 12, id.size);
    bio::putU64(rec + 20, p.byteOffset);
    bio::putU32(rec + 28, p.layoutHash);
    bio::putU64(rec + 32, p.timestampMs);
    bio::putU32(rec + 40, Crc32::compute(rec, 40));
}

// Intenta leer y validar un registro de `path`. true si es válido.
bool tryLoad(IFileSystem& fs, const char* path, const BookId& id, Progress& out) {
    if (!fs.exists(path)) return false;
    IFile* f = nullptr;
    if (!isOk(fs.open(path, OpenMode::Read, f))) return false;
    uint8_t rec[kRecSize];
    size_t got = 0;
    Result r = f->read(rec, kRecSize, got);
    f->close();
    delete f;
    if (!isOk(r) || got < kRecSize) return false;
    if (std::memcmp(rec, "ERPG", 4) != 0) return false;
    if (bio::getU16(rec + 4) != kVersion) return false;
    if (Crc32::compute(rec, 40) != bio::getU32(rec + 40)) return false;
    BookId fid{bio::getU32(rec + 8), bio::getU64(rec + 12)};
    if (!(fid == id)) return false;
    out.byteOffset = bio::getU64(rec + 20);
    out.layoutHash = bio::getU32(rec + 28);
    out.timestampMs = bio::getU64(rec + 32);
    return true;
}
}  // namespace

Result ProgressStore::save(IFileSystem& fs, const char* path, const BookId& id,
                           const Progress& p) {
    char tmp[cfg::kMaxPathLen], bak[cfg::kMaxPathLen];
    if (!makePath(path, ".tmp", tmp, sizeof(tmp)) ||
        !makePath(path, ".bak", bak, sizeof(bak))) {
        return Result::ErrInvalidArg;
    }

    uint8_t rec[kRecSize];
    encode(id, p, rec);

    // 1) Escribir el temporal y sincronizar.
    IFile* f = nullptr;
    Result r = fs.open(tmp, OpenMode::Write, f);
    if (!isOk(r)) return r;
    r = f->write(rec, kRecSize);
    if (isOk(r)) r = f->flush();
    f->close();
    delete f;
    if (!isOk(r)) { fs.remove(tmp); return r; }

    // 2) Mover el actual a .bak (si existe).
    if (fs.exists(path)) {
        fs.remove(bak);  // ignorar error si no existe
        r = fs.rename(path, bak);
        if (!isOk(r)) return r;
    }
    // 3) Renombrar el temporal al definitivo.
    r = fs.rename(tmp, path);
    return r;
}

Result ProgressStore::load(IFileSystem& fs, const char* path, const BookId& id,
                           Progress& out) {
    char bak[cfg::kMaxPathLen], tmp[cfg::kMaxPathLen];
    makePath(path, ".bak", bak, sizeof(bak));
    makePath(path, ".tmp", tmp, sizeof(tmp));
    if (tryLoad(fs, path, id, out)) return Result::Ok;
    if (tryLoad(fs, bak, id, out)) return Result::Ok;
    if (tryLoad(fs, tmp, id, out)) return Result::Ok;
    return Result::ErrNotFound;
}

}  // namespace ereader
