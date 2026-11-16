// utf8_decoder.cpp — Implementación del decodificador UTF-8 con estado.
#include "ereader/utf8_decoder.hpp"

#include "ereader/config.hpp"

namespace ereader {

namespace {
constexpr uint32_t kCR = 0x0D;
constexpr uint32_t kLF = 0x0A;
constexpr uint32_t kBOM = 0xFEFF;
}  // namespace

void Utf8Decoder::reset(bool atStart) {
    seqRemaining_ = 0;
    seqAcc_ = 0;
    seqMin_ = 0;
    seqStartOffset_ = 0;
    sawCr_ = false;
    atStart_ = atStart;
}

void Utf8Decoder::emit(uint32_t cp, uint64_t off, uint32_t* out,
                       uint64_t* outOffsets, size_t& outLen) {
    // 1) BOM: solo se descarta si es el primer codepoint del stream.
    if (atStart_) {
        atStart_ = false;
        if (cp == kBOM) {
            return;  // Se traga el BOM.
        }
    }

    // 2) Normalización de finales de línea a LF.
    if (cp == kCR) {
        sawCr_ = true;
        if (outOffsets) outOffsets[outLen] = off;
        out[outLen++] = kLF;  // CR -> LF.
        return;
    }
    if (cp == kLF) {
        if (sawCr_) {
            sawCr_ = false;  // Era la parte LF de un CRLF: ya se emitió el LF.
            return;
        }
        if (outOffsets) outOffsets[outLen] = off;
        out[outLen++] = kLF;
        return;
    }
    sawCr_ = false;
    if (outOffsets) outOffsets[outLen] = off;
    out[outLen++] = cp;
}

Result Utf8Decoder::decode(const uint8_t* in, size_t inLen, uint64_t inStartOffset,
                           uint32_t* out, uint64_t* outOffsets, size_t outCap,
                           size_t& outLen) {
    outLen = 0;
    if (outCap < inLen) {
        return Result::ErrInvalidArg;
    }

    size_t i = 0;
    while (i < inLen) {
        uint8_t b = in[i];
        uint64_t off = inStartOffset + i;

        if (seqRemaining_ == 0) {
            // Esperando un byte "lead".
            if (b < 0x80) {
                emit(b, off, out, outOffsets, outLen);
                ++i;
            } else if ((b & 0xE0) == 0xC0) {  // 110xxxxx -> 2 bytes
                seqAcc_ = b & 0x1Fu;
                seqRemaining_ = 1;
                seqMin_ = 0x80;
                seqStartOffset_ = off;
                ++i;
            } else if ((b & 0xF0) == 0xE0) {  // 1110xxxx -> 3 bytes
                seqAcc_ = b & 0x0Fu;
                seqRemaining_ = 2;
                seqMin_ = 0x800;
                seqStartOffset_ = off;
                ++i;
            } else if ((b & 0xF8) == 0xF0) {  // 11110xxx -> 4 bytes
                seqAcc_ = b & 0x07u;
                seqRemaining_ = 3;
                seqMin_ = 0x10000;
                seqStartOffset_ = off;
                ++i;
            } else {
                // Byte de continuación suelto (10xxxxxx) o 0xF8..0xFF: inválido.
                emit(cfg::kReplacementChar, off, out, outOffsets, outLen);
                ++i;
            }
        } else {
            // Esperando un byte de continuación 10xxxxxx.
            if ((b & 0xC0) == 0x80) {
                seqAcc_ = (seqAcc_ << 6) | (b & 0x3Fu);
                --seqRemaining_;
                ++i;
                if (seqRemaining_ == 0) {
                    // Secuencia completa: validar overlong, rango y sustitutos.
                    bool overlong = seqAcc_ < seqMin_;
                    bool surrogate = (seqAcc_ >= 0xD800 && seqAcc_ <= 0xDFFF);
                    bool tooBig = seqAcc_ > 0x10FFFF;
                    if (overlong || surrogate || tooBig) {
                        emit(cfg::kReplacementChar, seqStartOffset_, out, outOffsets, outLen);
                    } else {
                        emit(seqAcc_, seqStartOffset_, out, outOffsets, outLen);
                    }
                }
            } else {
                // Byte inesperado: la secuencia queda truncada -> U+FFFD, y el
                // byte actual se reprocesa como nuevo lead (NO se avanza i).
                emit(cfg::kReplacementChar, seqStartOffset_, out, outOffsets, outLen);
                seqRemaining_ = 0;
            }
        }
    }
    return Result::Ok;
}

size_t Utf8Decoder::finish(uint32_t* out, uint64_t* outOffsets, size_t outCap) {
    size_t outLen = 0;
    if (seqRemaining_ != 0 && outCap >= 1) {
        // Secuencia truncada al final del stream.
        emit(cfg::kReplacementChar, seqStartOffset_, out, outOffsets, outLen);
        seqRemaining_ = 0;
    }
    return outLen;
}

}  // namespace ereader
