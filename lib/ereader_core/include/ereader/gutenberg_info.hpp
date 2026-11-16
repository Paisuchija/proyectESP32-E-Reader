// gutenberg_info.hpp — Detección de cabecera/pie de Project Gutenberg.
//
// Detecta los marcadores (§7.2):
//   *** START OF THE  PROJECT GUTENBERG EBOOK ... ***
//   *** START OF THIS PROJECT GUTENBERG EBOOK ... ***
//   *** END   OF THE/THIS PROJECT GUTENBERG EBOOK ... ***
// sin distinguir mayúsculas, y extrae Title/Author/Language de la cabecera.
// Si el archivo no es de Gutenberg, el cuerpo es el archivo entero.
//
// Rendimiento: NO recorre el archivo completo (eso rompería el presupuesto de
// apertura <500 ms en SD). Escanea una ventana de cabecera y una de cola
// acotadas por config.
#pragma once

#include <cstdint>

#include "ereader/ifile.hpp"
#include "ereader/result.hpp"

namespace ereader {

constexpr size_t kGutenMetaLen = 128;  // Longitud máx. de Title/Author/Language.

struct GutenbergInfo {
    uint64_t bodyStart = 0;   // Offset del primer byte del cuerpo.
    uint64_t bodyEnd = 0;     // Offset de fin del cuerpo (exclusivo).
    bool isGutenberg = false; // true si se encontró el marcador START.
    char title[kGutenMetaLen] = {0};
    char author[kGutenMetaLen] = {0};
    char language[kGutenMetaLen] = {0};
};

// Analiza `file` y rellena `out`. Siempre deja bodyStart/bodyEnd coherentes
// (todo el archivo si no es Gutenberg). Devuelve ErrIO en fallo de lectura.
Result detectGutenberg(IFile& file, GutenbergInfo& out);

}  // namespace ereader
