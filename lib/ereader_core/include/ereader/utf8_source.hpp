// utf8_source.hpp — Fuente de codepoints UTF-8 sobre un IFile.
//
// Lee el rango [bodyStart, bodyEnd) de un archivo en bloques de
// kDecodeChunkBytes, lo decodifica con Utf8Decoder (estado conservado entre
// bloques) y entrega codepoints con su offset de origen. No carga el archivo
// en RAM (regla §6).
#pragma once

#include <cstdint>

#include "ereader/codepoint.hpp"
#include "ereader/config.hpp"
#include "ereader/ifile.hpp"
#include "ereader/utf8_decoder.hpp"

namespace ereader {

class Utf8Source : public ICodepointSource {
public:
    Utf8Source() = default;

    // Inicializa sobre [bodyStart, bodyEnd) de `file` (no toma propiedad).
    Result init(IFile* file, uint64_t bodyStart, uint64_t bodyEnd);

    Result next(Cp& out) override;
    Result peek(Cp& out) override;
    Result seekTo(uint64_t byteOffset) override;

    uint64_t bodyStart() const { return bodyStart_; }
    uint64_t bodyEnd() const { return bodyEnd_; }

private:
    // Rellena el buffer de codepoints leyendo y decodificando el siguiente
    // bloque. Devuelve false si no hay más datos.
    bool fill();

    IFile* file_ = nullptr;
    uint64_t bodyStart_ = 0;
    uint64_t bodyEnd_ = 0;
    uint64_t readPos_ = 0;   // Próximo byte a leer del archivo.

    Utf8Decoder dec_;
    uint8_t block_[cfg::kDecodeChunkBytes];
    uint32_t cps_[cfg::kDecodeChunkBytes];
    uint64_t offs_[cfg::kDecodeChunkBytes];
    size_t cpCount_ = 0;
    size_t cpPos_ = 0;
    bool eofReached_ = false;  // Ya se leyó hasta bodyEnd_.
    bool finished_ = false;    // Ya se llamó a decoder.finish().
};

}  // namespace ereader
