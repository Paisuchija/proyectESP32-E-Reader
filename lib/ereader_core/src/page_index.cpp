// page_index.cpp — Implementación del índice de páginas.
#include "ereader/page_index.hpp"

#include <cstring>

#include "ereader/byteio.hpp"
#include "ereader/config.hpp"
#include "ereader/crc32.hpp"

namespace ereader {

namespace {
constexpr size_t kHeaderSize = 48;
constexpr size_t kEntrySize = 10;
constexpr uint16_t kVersion = 1;
constexpr uint16_t kFlagComplete = 0x0001;
constexpr uint32_t kFlushEveryPages = 256;  // Checkpoint de pageCount en disco.

uint64_t entryFileOffset(uint32_t i) {
    return kHeaderSize + static_cast<uint64_t>(i) * kEntrySize;
}
}  // namespace

Result PageIndex::readHeader() {
    uint8_t h[kHeaderSize];
    Result r = file_->seek(0, Seek::Set);
    if (!isOk(r)) return r;
    size_t got = 0;
    r = file_->read(h, kHeaderSize, got);
    if (!isOk(r)) return r;
    if (got < kHeaderSize) { state_ = State::Mismatch; return Result::Ok; }

    if (std::memcmp(h, "ERIX", 4) != 0 || bio::getU16(h + 4) != kVersion) {
        state_ = State::Mismatch;
        return Result::Ok;
    }
    uint16_t flags = bio::getU16(h + 6);
    BookId fid{bio::getU32(h + 8), bio::getU64(h + 12)};
    uint32_t lh = bio::getU32(h + 20);
    if (!(fid == id_) || lh != layoutHash_) {
        state_ = State::Mismatch;
        return Result::Ok;
    }
    bodyStart_ = bio::getU64(h + 24);
    bodyEnd_ = bio::getU64(h + 32);
    pageCount_ = bio::getU32(h + 40);
    uint32_t storedCrc = bio::getU32(h + 44);

    // Comprobar que el archivo tiene al menos las entradas declaradas.
    uint64_t need = entryFileOffset(pageCount_);
    if (file_->size() < need) { state_ = State::Mismatch; return Result::Ok; }

    if (flags & kFlagComplete) {
        // Verificar CRC sobre [0,44) + entradas.
        Crc32 crc;
        crc.update(h, 44);
        uint8_t buf[cfg::kReadBlockBytes];
        r = file_->seek(kHeaderSize, Seek::Set);
        if (!isOk(r)) return r;
        uint64_t remaining = static_cast<uint64_t>(pageCount_) * kEntrySize;
        while (remaining > 0) {
            size_t want = static_cast<size_t>(
                remaining < sizeof(buf) ? remaining : sizeof(buf));
            size_t gr = 0;
            r = file_->read(buf, want, gr);
            if (!isOk(r)) return r;
            if (gr == 0) break;
            crc.update(buf, gr);
            remaining -= gr;
        }
        state_ = (crc.value() == storedCrc) ? State::Complete : State::Mismatch;
    } else {
        state_ = State::Partial;  // válido pero incompleto (reanudable, aun con 0)
    }
    if (pageCount_ > 0) {
        uint16_t fl;
        getEntry(pageCount_ - 1, lastOffset_, fl);
    } else {
        lastOffset_ = bodyStart_;
    }
    return Result::Ok;
}

Result PageIndex::openRead(IFileSystem& fs, const char* path, const BookId& id,
                           uint32_t layoutHash) {
    close();
    id_ = id;
    layoutHash_ = layoutHash;
    writable_ = false;
    if (!fs.exists(path)) { state_ = State::Absent; return Result::Ok; }
    Result r = fs.open(path, OpenMode::Read, file_);
    if (!isOk(r)) { state_ = State::Absent; return Result::Ok; }
    return readHeader();
}

Result PageIndex::getEntry(uint32_t i, uint64_t& offset, uint16_t& flags) {
    if (!file_ || i >= pageCount_) return Result::ErrInvalidArg;
    uint8_t e[kEntrySize];
    Result r = file_->seek(static_cast<int64_t>(entryFileOffset(i)), Seek::Set);
    if (!isOk(r)) return r;
    size_t got = 0;
    r = file_->read(e, kEntrySize, got);
    if (!isOk(r)) return r;
    if (got < kEntrySize) return Result::ErrCorrupt;
    offset = bio::getU64(e);
    flags = bio::getU16(e + 8);
    return Result::Ok;
}

Result PageIndex::findPage(uint64_t byteOffset, uint32_t& pageIndex) {
    if (pageCount_ == 0) return Result::ErrNotFound;
    uint64_t off0, offN;
    uint16_t fl;
    getEntry(0, off0, fl);
    if (byteOffset < off0) return Result::ErrNotFound;
    // Búsqueda binaria del mayor i con entry[i].offset <= byteOffset.
    uint32_t lo = 0, hi = pageCount_ - 1, ans = 0;
    while (lo <= hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        getEntry(mid, offN, fl);
        if (offN <= byteOffset) {
            ans = mid;
            if (mid == pageCount_ - 1) break;
            lo = mid + 1;
        } else {
            if (mid == 0) break;
            hi = mid - 1;
        }
    }
    pageIndex = ans;
    return Result::Ok;
}

Result PageIndex::createOrResume(IFileSystem& fs, const char* path, const BookId& id,
                                 uint32_t layoutHash, uint64_t bodyStart,
                                 uint64_t bodyEnd, uint32_t& resumeFromPage,
                                 uint64_t& resumeFromOffset, bool& resumeParaStart) {
    close();
    id_ = id;
    layoutHash_ = layoutHash;
    bodyStart_ = bodyStart;
    bodyEnd_ = bodyEnd;
    resumeFromPage = 0;
    resumeFromOffset = bodyStart;
    resumeParaStart = true;  // la página 0 siempre inicia párrafo

    // ¿Hay un índice parcial reanudable? Debe coincidir libro, layout y cuerpo.
    uint32_t resumePages = 0;
    uint64_t resumeOff = bodyStart;
    uint16_t resumeFlags = kPageFlagParaStart;
    bool resume = false;
    if (fs.exists(path)) {
        PageIndex probe;
        probe.openRead(fs, path, id, layoutHash);
        if (probe.state() == State::Partial && probe.bodyStart() == bodyStart &&
            probe.bodyEnd() == bodyEnd && probe.pageCount() > 0) {
            // Reanudar reescribiendo la ÚLTIMA entrada (repaginar desde su offset
            // reproduce esa misma página de forma determinista, sin duplicar).
            uint32_t n = probe.pageCount();
            uint64_t off; uint16_t fl;
            if (isOk(probe.getEntry(n - 1, off, fl))) {
                resume = true;
                resumePages = n - 1;       // conservamos n-1 entradas intactas
                resumeOff = off;           // repaginamos desde la última
                resumeFlags = fl;
            }
        }
        probe.close();
    }

    Result r;
    if (resume) {
        r = fs.open(path, OpenMode::Update, file_);
        if (!isOk(r)) return r;
        writable_ = true;
        pageCount_ = resumePages;
        lastOffset_ = resumeOff;
        resumeFromPage = resumePages;
        resumeFromOffset = resumeOff;
        resumeParaStart = (resumeFlags & kPageFlagParaStart) != 0;
        // Colocar el cursor de escritura donde se reescribirá la entrada.
        r = file_->seek(static_cast<int64_t>(entryFileOffset(pageCount_)), Seek::Set);
        if (!isOk(r)) return r;
        state_ = State::Partial;
    } else {
        r = fs.open(path, OpenMode::Write, file_);
        if (!isOk(r)) return r;
        writable_ = true;
        pageCount_ = 0;
        lastOffset_ = bodyStart_;
        flushHeader();  // header inicial (incompleto)
        r = file_->seek(kHeaderSize, Seek::Set);
        if (!isOk(r)) return r;
        state_ = State::Partial;
    }
    sinceFlush_ = 0;
    return Result::Ok;
}

Result PageIndex::appendPage(uint64_t offset, uint16_t flags) {
    if (!file_ || !writable_) return Result::ErrInvalidArg;
    uint8_t e[kEntrySize];
    bio::putU64(e, offset);
    bio::putU16(e + 8, flags);
    // El cursor de escritura ya está colocado tras la entrada previa.
    Result r = file_->write(e, kEntrySize);
    if (!isOk(r)) return r;
    lastOffset_ = offset;
    ++pageCount_;
    if (++sinceFlush_ >= kFlushEveryPages) {
        sinceFlush_ = 0;
        // flushHeader mueve el cursor; hay que restaurarlo al final de entradas.
        flushHeader();
        file_->seek(static_cast<int64_t>(entryFileOffset(pageCount_)), Seek::Set);
    }
    return Result::Ok;
}

Result PageIndex::flushHeader() {
    if (!file_ || !writable_) return Result::ErrInvalidArg;
    uint8_t h[kHeaderSize];
    std::memset(h, 0, sizeof(h));
    std::memcpy(h, "ERIX", 4);
    bio::putU16(h + 4, kVersion);
    bio::putU16(h + 6, 0);  // incompleto
    bio::putU32(h + 8, id_.crc);
    bio::putU64(h + 12, id_.size);
    bio::putU32(h + 20, layoutHash_);
    bio::putU64(h + 24, bodyStart_);
    bio::putU64(h + 32, bodyEnd_);
    bio::putU32(h + 40, pageCount_);
    bio::putU32(h + 44, 0);
    Result r = file_->seek(0, Seek::Set);
    if (!isOk(r)) return r;
    r = file_->write(h, kHeaderSize);
    if (!isOk(r)) return r;
    return file_->flush();
}

Result PageIndex::finalize() {
    if (!file_ || !writable_) return Result::ErrInvalidArg;
    // Construir el header final (completo) y calcular el CRC sobre [0,44) +
    // entradas (releídas del disco).
    uint8_t h[kHeaderSize];
    std::memset(h, 0, sizeof(h));
    std::memcpy(h, "ERIX", 4);
    bio::putU16(h + 4, kVersion);
    bio::putU16(h + 6, kFlagComplete);
    bio::putU32(h + 8, id_.crc);
    bio::putU64(h + 12, id_.size);
    bio::putU32(h + 20, layoutHash_);
    bio::putU64(h + 24, bodyStart_);
    bio::putU64(h + 32, bodyEnd_);
    bio::putU32(h + 40, pageCount_);

    Crc32 crc;
    crc.update(h, 44);
    uint8_t buf[cfg::kReadBlockBytes];
    Result r = file_->seek(kHeaderSize, Seek::Set);
    if (!isOk(r)) return r;
    uint64_t remaining = static_cast<uint64_t>(pageCount_) * kEntrySize;
    while (remaining > 0) {
        size_t want = static_cast<size_t>(remaining < sizeof(buf) ? remaining : sizeof(buf));
        size_t got = 0;
        r = file_->read(buf, want, got);
        if (!isOk(r)) return r;
        if (got == 0) break;
        crc.update(buf, got);
        remaining -= got;
    }
    bio::putU32(h + 44, crc.value());
    r = file_->seek(0, Seek::Set);
    if (!isOk(r)) return r;
    r = file_->write(h, kHeaderSize);
    if (!isOk(r)) return r;
    r = file_->flush();
    if (!isOk(r)) return r;
    state_ = State::Complete;
    return Result::Ok;
}

void PageIndex::close() {
    if (file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
    }
    state_ = State::Absent;
    writable_ = false;
    pageCount_ = 0;
    sinceFlush_ = 0;
}

}  // namespace ereader
