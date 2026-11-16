// codepoint.hpp — Tipos base del flujo de texto decodificado.
#pragma once

#include <cstdint>

#include "ereader/result.hpp"

namespace ereader {

// Un codepoint Unicode junto con el offset en bytes de su primer byte dentro
// del archivo de origen. El offset es lo que identifica a una página (§7.2),
// por eso viaja con el codepoint a lo largo de todo el pipeline.
struct Cp {
    uint32_t cp;       // Valor Unicode.
    uint64_t offset;   // Offset en bytes en el archivo de origen.
};

// Fuente de codepoints: el pipeline de maquetación consume de aquí.
class ICodepointSource {
public:
    virtual ~ICodepointSource() = default;

    // Devuelve Ok con `out` relleno, Result::End al agotar la fuente, o un
    // error de E/S.
    virtual Result next(Cp& out) = 0;

    // Mira el próximo codepoint sin consumirlo (mismo valor que devolverá el
    // siguiente next()). End si no hay más.
    virtual Result peek(Cp& out) = 0;

    // Reposiciona la fuente para empezar a emitir desde `byteOffset` (que debe
    // ser el inicio de un codepoint, normalmente el inicio de una página).
    virtual Result seekTo(uint64_t byteOffset) = 0;
};

}  // namespace ereader
