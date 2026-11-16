// utf8_source.cpp — Implementación de Utf8Source.
#include "ereader/utf8_source.hpp"

namespace ereader {

Result Utf8Source::init(IFile* file, uint64_t bodyStart, uint64_t bodyEnd) {
    if (!file || bodyEnd < bodyStart) return Result::ErrInvalidArg;
    file_ = file;
    bodyStart_ = bodyStart;
    bodyEnd_ = bodyEnd;
    return seekTo(bodyStart);
}

Result Utf8Source::seekTo(uint64_t byteOffset) {
    if (!file_) return Result::ErrInvalidArg;
    if (byteOffset < bodyStart_) byteOffset = bodyStart_;
    if (byteOffset > bodyEnd_) byteOffset = bodyEnd_;
    Result r = file_->seek(static_cast<int64_t>(byteOffset), Seek::Set);
    if (!isOk(r)) return r;
    readPos_ = byteOffset;
    // El BOM solo se descarta si arrancamos en el inicio real del archivo.
    dec_.reset(byteOffset == 0);
    cpCount_ = 0;
    cpPos_ = 0;
    eofReached_ = false;
    finished_ = false;
    return Result::Ok;
}

bool Utf8Source::fill() {
    cpCount_ = 0;
    cpPos_ = 0;

    while (cpCount_ == 0) {
        if (!eofReached_) {
            // Leer el siguiente bloque sin pasar de bodyEnd_.
            uint64_t remaining = bodyEnd_ - readPos_;
            size_t want = static_cast<size_t>(
                remaining < cfg::kDecodeChunkBytes ? remaining : cfg::kDecodeChunkBytes);
            if (want == 0) {
                eofReached_ = true;
                continue;
            }
            size_t got = 0;
            Result r = file_->read(block_, want, got);
            if (!isOk(r)) return false;
            if (got == 0) {
                eofReached_ = true;
                continue;
            }
            uint64_t blockStart = readPos_;
            readPos_ += got;
            dec_.decode(block_, got, blockStart, cps_, offs_, cfg::kDecodeChunkBytes, cpCount_);
            // Si el bloque solo contenía bytes de una secuencia incompleta,
            // cpCount_ puede ser 0: el bucle vuelve a leer.
        } else if (!finished_) {
            finished_ = true;
            cpCount_ = dec_.finish(cps_, offs_, cfg::kDecodeChunkBytes);
            if (cpCount_ == 0) return false;
        } else {
            return false;
        }
    }
    return true;
}

Result Utf8Source::next(Cp& out) {
    if (cpPos_ >= cpCount_) {
        if (!fill()) return Result::End;
    }
    out.cp = cps_[cpPos_];
    out.offset = offs_[cpPos_];
    ++cpPos_;
    return Result::Ok;
}

Result Utf8Source::peek(Cp& out) {
    if (cpPos_ >= cpCount_) {
        if (!fill()) return Result::End;
    }
    out.cp = cps_[cpPos_];
    out.offset = offs_[cpPos_];
    return Result::Ok;
}

}  // namespace ereader
