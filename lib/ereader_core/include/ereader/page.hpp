// page.hpp — Estructuras de una página ya maquetada.
//
// Buffers de tamaño fijo (regla §6: nada de asignación por página). Una Page es
// grande; se instancia UNA y se reutiliza (el indexado solo lee offset/flags,
// el render lee los glifos con su x/y).
#pragma once

#include <cstdint>

#include "ereader/config.hpp"

namespace ereader {

// Un glifo colocado: codepoint y su posición x (px desde el margen izquierdo).
struct Glyph {
    uint32_t cp;
    int32_t x;
};

// Una línea maquetada.
struct Line {
    Glyph glyphs[cfg::kMaxGlyphsPerLine];
    uint16_t count = 0;
    int32_t y = 0;                  // Y del borde superior de la línea (px).
    int32_t width = 0;              // Ancho ocupado (px).
    uint64_t startOffset = 0;       // Offset de origen del primer glifo.
    bool startsParagraph = false;   // Primera línea de un párrafo (sangría).
    bool paragraphGapBefore = false;// Dejar espacio de párrafo por encima.
};

// Una página: conjunto de líneas y su identidad.
struct Page {
    Line lines[cfg::kMaxLinesPerPage];
    uint16_t lineCount = 0;
    uint64_t startOffset = 0;        // Offset de origen del primer carácter.
    bool startsParagraph = false;    // La página empieza un párrafo.

    void clear() { lineCount = 0; startOffset = 0; startsParagraph = false; }
};

}  // namespace ereader
