// progress.hpp — Progreso de lectura con escritura atómica (§6).
//
// El progreso se guarda como OFFSET EN BYTES (no número de página) para que
// sobreviva a cambios de fuente/geometría (§7.2). Escritura atómica:
//   1. escribir en <path>.tmp y hacer flush (fsync);
//   2. si existe <path>, moverlo a <path>.bak;
//   3. renombrar <path>.tmp -> <path>.
// Si se corta la energía entre (2) y (3), la recuperación usa <path>.bak. Cada
// registro lleva CRC32 para descartar archivos corruptos.
//
// Formato (.prg), 44 bytes little-endian:
//   0 magic "ERPG" | 4 version u16 | 6 flags u16 | 8 bookCrc u32 |
//   12 bookSize u64 | 20 byteOffset u64 | 28 layoutHash u32 |
//   32 timestampMs u64 | 40 crc32 u32 (sobre [0,40))
#pragma once

#include <cstdint>

#include "ereader/book_id.hpp"
#include "ereader/ifilesystem.hpp"
#include "ereader/result.hpp"

namespace ereader {

struct Progress {
    uint64_t byteOffset = 0;   // Posición de lectura en bytes dentro del cuerpo.
    uint32_t layoutHash = 0;   // Layout con el que se guardó (informativo).
    uint64_t timestampMs = 0;  // Marca de tiempo (0 si no disponible).
};

class ProgressStore {
public:
    // Guarda de forma atómica. `path` no lleva sufijo (se derivan .tmp/.bak).
    static Result save(IFileSystem& fs, const char* path, const BookId& id,
                       const Progress& p);

    // Carga, probando <path>, luego <path>.bak, luego <path>.tmp. Verifica CRC
    // y que el BookId coincide. ErrNotFound si no hay ninguno válido.
    static Result load(IFileSystem& fs, const char* path, const BookId& id,
                       Progress& out);
};

}  // namespace ereader
