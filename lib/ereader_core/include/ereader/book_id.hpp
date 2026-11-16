// book_id.hpp — Identificador estable de un libro (§7.2 BookId).
//
// CRC32 de los primeros 4 KB + tamaño del archivo. Sobrevive a un cambio de
// nombre y distingue libros distintos con alta probabilidad. Se usa para
// nombrar las cachés (índice, progreso, marcadores) en /sys/<book_id>/.
#pragma once

#include <cstdint>

#include "ereader/ifile.hpp"
#include "ereader/result.hpp"

namespace ereader {

struct BookId {
    uint32_t crc = 0;    // CRC32 de los primeros kBookIdPrefixBytes.
    uint64_t size = 0;   // Tamaño total del archivo.

    bool operator==(const BookId& o) const { return crc == o.crc && size == o.size; }

    // Escribe una representación hex estable en `buf` (p. ej. "a1b2c3d4-0012ab").
    // `cap` debe ser >= 24. Devuelve buf.
    char* toString(char* buf, size_t cap) const;
};

// Calcula el BookId de `file` (deja el cursor donde quede).
Result computeBookId(IFile& file, BookId& out);

}  // namespace ereader
