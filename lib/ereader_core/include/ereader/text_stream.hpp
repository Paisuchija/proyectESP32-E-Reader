// text_stream.hpp — Tokens de texto y filtro de reflow.
//
// El reflow (§7.2) transforma el flujo de codepoints en tokens que el motor de
// líneas entiende. Modos:
//  - ON (por defecto): un salto de línea simple dentro de un párrafo se une con
//    un espacio; una línea en blanco (o solo con espacios) es salto de párrafo.
//    Colapsa todo bloque de espacios/saltos entre dos glifos en UN token.
//  - OFF: cada '\n' es un salto de línea forzado (preserva poesía, p. ej. la
//    Divina Comedia). Las líneas en blanco se conservan.
//
// Límites del heurístico (documentar en ARCHITECTURE.md): el reflow ON no
// distingue encabezados, listas ni versos; para esos textos se usa reflow OFF.
#pragma once

#include <cstdint>

#include "ereader/codepoint.hpp"
#include "ereader/result.hpp"

namespace ereader {

enum class TokKind : uint8_t {
    Glyph,      // Codepoint visible (cp válido).
    Nbsp,       // U+00A0: glifo de espacio que NO permite corte.
    Space,      // Separador suave: oportunidad de corte, colapsable.
    LineBreak,  // Salto de línea forzado (reflow OFF).
    ParaBreak,  // Fin de párrafo (reflow ON).
    End,        // Fin del cuerpo.
};

struct Token {
    TokKind kind;
    uint32_t cp;      // Solo para Glyph/Nbsp.
    uint64_t offset;  // Offset de origen del primer byte del token.
};

// Fuente de tokens con lookahead de un token.
class ITokenSource {
public:
    virtual ~ITokenSource() = default;
    virtual Result next(Token& out) = 0;
    virtual Result peek(Token& out) = 0;
};

// Aplica reflow sobre una fuente de codepoints.
class ReflowFilter : public ITokenSource {
public:
    // `src` no es propiedad de este objeto. `reflow` = modo ON.
    void init(ICodepointSource* src, bool reflow);

    Result next(Token& out) override;
    Result peek(Token& out) override;

private:
    // Produce el siguiente token (sin cache de peek).
    Result produce(Token& out);

    ICodepointSource* src_ = nullptr;
    bool reflow_ = true;
    bool emittedContent_ = false;  // ¿Se emitió ya algún glifo? (para recortar inicio)

    bool hasPeek_ = false;
    Token peek_{};
};

}  // namespace ereader
