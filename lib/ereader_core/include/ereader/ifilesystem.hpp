// ifilesystem.hpp — Abstracción de sistema de archivos.
//
// Portabilidad: StdioFileSystem (native) y SdFileSystem (DevKit). El núcleo
// solo depende de esta interfaz, nunca de SD.h ni de <filesystem>.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ereader/ifile.hpp"
#include "ereader/result.hpp"

namespace ereader {

// Modo de apertura. Se mantiene minimalista a propósito.
enum class OpenMode : uint8_t {
    Read,    // Lectura. Falla si no existe.
    Write,   // Escritura (y lectura): crea o trunca.
    Update,  // Lectura+escritura sobre un archivo existente (sin truncar).
};

// Entrada de directorio que se pasa al callback de listDir().
struct DirEntry {
    const char* name;  // Nombre del archivo (no la ruta completa). Válido solo durante el callback.
    uint64_t size;     // Tamaño en bytes.
    bool isDir;         // true si es un directorio.
};

// Callback de listado. Devolver false aborta el recorrido.
using DirCallback = bool (*)(const DirEntry& entry, void* user);

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    // Abre un archivo. Devuelve el puntero en `out` (propiedad del llamante:
    // debe hacer close() y delete, o usarlo con un wrapper RAII).
    virtual Result open(const char* path, OpenMode mode, IFile*& out) = 0;

    // ¿Existe la ruta (archivo o directorio)?
    virtual bool exists(const char* path) = 0;

    // Borra un archivo.
    virtual Result remove(const char* path) = 0;

    // Renombra/mueve. Debe ser atómico en el mismo sistema de archivos
    // (requisito de la escritura atómica de progreso, §6).
    virtual Result rename(const char* from, const char* to) = 0;

    // Crea un directorio (no falla si ya existe).
    virtual Result mkdir(const char* path) = 0;

    // Recorre las entradas de un directorio invocando `cb` por cada una.
    virtual Result listDir(const char* path, DirCallback cb, void* user) = 0;

    // Bytes libres en el medio. 0 si no se puede determinar.
    virtual uint64_t freeBytes() = 0;
};

}  // namespace ereader
