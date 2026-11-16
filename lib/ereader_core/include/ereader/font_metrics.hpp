// font_metrics.hpp — Métricas tipográficas (§7.2 FontMetrics).
//
// El motor de líneas/páginas solo conoce esta interfaz: ancho de avance por
// codepoint y métricas verticales, todo en píxeles. Hoy la implementan
// FixedWidthMetrics (tests) y TableMetrics (tabla generada desde una TTF OFL).
// Mañana la misma tabla alimentará el renderer de glifos 4 bpp del S3.
#pragma once

#include <cstdint>

namespace ereader {

class FontMetrics {
public:
    virtual ~FontMetrics() = default;

    // Ancho de avance del codepoint en px. Un codepoint sin glifo debe
    // devolver un ancho de reemplazo (no 0).
    virtual int32_t advance(uint32_t cp) const = 0;

    // Alto de línea (de línea base a línea base sin interlineado extra), px.
    virtual int32_t lineHeight() const = 0;
    virtual int32_t ascent() const = 0;
    virtual int32_t descent() const = 0;

    // Identificador estable de la fuente+tamaño. Entra en el layout_hash del
    // índice: si cambia, el índice se invalida y se recrea (§7.2 PageIndex).
    virtual uint32_t id() const = 0;
};

// Métricas de ancho fijo: todos los glifos miden lo mismo. Útil para tests
// deterministas y para prosa monoespaciada.
class FixedWidthMetrics : public FontMetrics {
public:
    FixedWidthMetrics(int32_t advance, int32_t lineHeight, int32_t ascent,
                      int32_t descent)
        : advance_(advance), lineHeight_(lineHeight), ascent_(ascent),
          descent_(descent) {}

    int32_t advance(uint32_t) const override { return advance_; }
    int32_t lineHeight() const override { return lineHeight_; }
    int32_t ascent() const override { return ascent_; }
    int32_t descent() const override { return descent_; }
    uint32_t id() const override {
        // Identificador derivado de los parámetros (suficiente para tests).
        return 0xF1000000u ^ static_cast<uint32_t>(advance_) ^
               (static_cast<uint32_t>(lineHeight_) << 8);
    }

private:
    int32_t advance_, lineHeight_, ascent_, descent_;
};

}  // namespace ereader
