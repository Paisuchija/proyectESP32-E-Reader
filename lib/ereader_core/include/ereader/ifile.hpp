// ifile.hpp — Abstracción de archivo de solo lectura/escritura por bloques.
//
// El núcleo nunca toca stdio ni la API de la SD directamente: habla con IFile.
// En `native` la implementa StdioFile; en la DevKit la implementará SdFile
// (lib/platform_devkit). Así el núcleo es portable (§4, §7.2).
#pragma once

#include <cstddef>
#include <cstdint>

#include "ereader/result.hpp"

namespace ereader {

// Origen del desplazamiento en seek(), equivalente a SEEK_SET/CUR/END.
enum class Seek : uint8_t { Set, Cur, End };

class IFile {
public:
    virtual ~IFile() = default;

    // Lee hasta `len` bytes en `out`. Devuelve en `read` los bytes leídos
    // (0 en EOF). Un read corto NO es error: hay que reintentar hasta End.
    virtual Result read(void* out, size_t len, size_t& read) = 0;

    // Escribe `len` bytes. Devuelve ErrIO si no se pudieron escribir todos.
    virtual Result write(const void* data, size_t len) = 0;

    // Reposiciona el cursor. `whence` indica el origen.
    virtual Result seek(int64_t offset, Seek whence) = 0;

    // Posición actual del cursor en bytes desde el inicio.
    virtual uint64_t tell() const = 0;

    // Tamaño total del archivo en bytes.
    virtual uint64_t size() const = 0;

    // Fuerza el volcado a disco de lo escrito (fsync). Clave para escrituras
    // atómicas: hay que sincronizar el .tmp antes del rename (§6).
    virtual Result flush() = 0;

    // Cierra el archivo. Idempotente.
    virtual void close() = 0;
};

}  // namespace ereader
