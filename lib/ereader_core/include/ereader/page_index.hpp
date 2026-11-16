// page_index.hpp — Índice binario de páginas en disco (§7.2 PageIndex).
//
// Formato (little-endian). Cabecera fija de 48 bytes:
//   0  magic[4] = "ERIX"
//   4  version  u16 (=1)
//   6  flags    u16 (bit0 = completo)
//   8  bookCrc  u32
//   12 bookSize u64
//   20 layoutHash u32
//   24 bodyStart  u64
//   32 bodyEnd    u64
//   40 pageCount  u32
//   44 crc32      u32 (CRC de los bytes [0,44) + todas las entradas; válido
//                      solo cuando el índice está completo)
// Entradas (desde el offset 48), pageCount × 10 bytes:
//   +0 offset u64   (offset de origen del primer carácter de la página)
//   +8 flags  u16   (bit0 = la página empieza un párrafo)
//
// Propiedades (§7.2, §10):
//  - Construcción incremental y reanudable (pageCount se actualiza a medida).
//  - layout_hash distinto -> índice no válido (se recrea).
//  - CRC corrupto en un índice "completo" -> se reconstruye.
#pragma once

#include <cstdint>

#include "ereader/book_id.hpp"
#include "ereader/ifilesystem.hpp"
#include "ereader/result.hpp"

namespace ereader {

constexpr uint16_t kPageFlagParaStart = 0x0001;

class PageIndex {
public:
    enum class State : uint8_t {
        Absent,    // No existe o no se pudo abrir.
        Mismatch,  // Existe pero es de otro libro/layout/versión -> no usar.
        Partial,   // Válido pero incompleto -> reanudable.
        Complete,  // Válido y completo (CRC verificado).
    };

    ~PageIndex() { close(); }

    // Abre el índice para LECTURA y lo valida contra (id, layoutHash).
    // Siempre deja `state()` coherente; devuelve Ok salvo error de E/S duro.
    Result openRead(IFileSystem& fs, const char* path, const BookId& id,
                    uint32_t layoutHash);

    State state() const { return state_; }
    uint32_t pageCount() const { return pageCount_; }
    bool complete() const { return state_ == State::Complete; }
    uint64_t bodyStart() const { return bodyStart_; }
    uint64_t bodyEnd() const { return bodyEnd_; }

    // Lee la entrada i (0-based).
    Result getEntry(uint32_t i, uint64_t& offset, uint16_t& flags);

    // Índice de la página cuyo offset es el mayor <= byteOffset. ErrNotFound
    // si no hay entradas o byteOffset < primera.
    Result findPage(uint64_t byteOffset, uint32_t& pageIndex);

    // --- Construcción ---
    // Crea un índice nuevo (trunca) o reanuda uno parcial válido. Devuelve en
    // `resumeFromPage`/`resumeFromOffset` desde dónde continuar paginando
    // (0 / bodyStart si se empieza de cero).
    Result createOrResume(IFileSystem& fs, const char* path, const BookId& id,
                          uint32_t layoutHash, uint64_t bodyStart, uint64_t bodyEnd,
                          uint32_t& resumeFromPage, uint64_t& resumeFromOffset,
                          bool& resumeParaStart);

    // Añade una entrada de página. Actualiza pageCount en memoria; el header en
    // disco se refresca cada cierto número de páginas y en finalize().
    Result appendPage(uint64_t offset, uint16_t flags);

    // Marca el índice como completo: refresca el header y escribe el CRC.
    Result finalize();

    // Refresca pageCount en el header en disco (checkpoint de reanudación).
    Result flushHeader();

    void close();

private:
    Result readHeader();

    IFile* file_ = nullptr;
    State state_ = State::Absent;
    bool writable_ = false;

    BookId id_{};
    uint32_t layoutHash_ = 0;
    uint64_t bodyStart_ = 0;
    uint64_t bodyEnd_ = 0;
    uint32_t pageCount_ = 0;
    uint64_t lastOffset_ = 0;  // offset de la última entrada (para reanudar).
    uint32_t sinceFlush_ = 0;
};

}  // namespace ereader
