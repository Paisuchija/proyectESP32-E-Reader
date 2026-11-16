// layout.hpp — Motor de líneas y paginación (§7.2 LineBreaker + Paginator).
//
// Convierte un flujo de tokens (post-reflow) en páginas maquetadas:
//  - LineBreaker voraz por ancho en píxeles: corta en espacios, NBSP no corta,
//    una palabra más larga que la línea se parte a la fuerza, colapsa espacios.
//  - Paginación por alto en píxeles, con interlineado y espacio de párrafo.
//
// DETERMINISMO (§7.2, requisito con test): paginar desde el offset de cualquier
// página produce exactamente las mismas páginas que paginar en continuo desde
// el inicio. Esto se garantiza porque:
//   * una página siempre empieza en el primer glifo de una línea (sin espacios
//     colgando por delante, que el reflow/breaker ya recortan);
//   * la sangría/estado de "inicio de párrafo" de la primera línea NO se deduce
//     del contexto previo, sino que se inyecta (flag almacenado en el índice);
//   * el empaquetado vertical arranca siempre en el margen superior.
#pragma once

#include <cstdint>

#include "ereader/config.hpp"
#include "ereader/font_metrics.hpp"
#include "ereader/geometry.hpp"
#include "ereader/page.hpp"
#include "ereader/text_stream.hpp"

namespace ereader {

class LayoutEngine {
public:
    LayoutEngine() = default;

    // `toks` y `fm` no son propiedad de este objeto. `firstStartsParagraph`
    // indica si la primera línea que se produzca es inicio de párrafo (true al
    // abrir el libro; el flag almacenado al repaginar desde una página).
    void init(ITokenSource* toks, const Geometry& geo, const FontMetrics* fm,
              bool firstStartsParagraph);

    // Produce la siguiente página. Result::End si no hay más contenido.
    Result nextPage(Page& out);

    // Produce la siguiente línea (expuesto para tests). Result::End al terminar.
    Result nextLine(Line& out);

    // Tras nextPage(): ¿hay una página siguiente? y su offset/flag de inicio.
    // Permite a ReaderSession conocer dónde empieza la página siguiente sin
    // volver a maquetar.
    bool hasNextPage() const { return haveCarry_; }
    uint64_t nextPageOffset() const { return haveCarry_ ? carryLine_.startOffset : 0; }
    bool nextPageStartsParagraph() const {
        return haveCarry_ ? carryLine_.startsParagraph : false;
    }

private:
    // Acumula el siguiente "word" (glifos+NBSP contiguos) en los buffers w*.
    // Devuelve el token de control que terminó la palabra (Space/LineBreak/
    // ParaBreak/End) ya consumido, o Glyph si la palabra llenó el buffer.
    void readWord();

    ITokenSource* toks_ = nullptr;
    const FontMetrics* fm_ = nullptr;
    Geometry geo_{};
    int32_t spaceAdvance_ = 0;

    bool pendingParaStart_ = true;  // La próxima línea inicia párrafo.
    bool pendingGap_ = false;       // La próxima línea lleva espacio de párrafo.

    // Palabra pendiente de colocar (recién leída o arrastrada de la línea previa).
    uint32_t wCp_[cfg::kMaxGlyphsPerLine];
    int32_t  wAdv_[cfg::kMaxGlyphsPerLine];
    uint64_t wOff_[cfg::kMaxGlyphsPerLine];
    int      wLen_ = 0;
    int32_t  wWidth_ = 0;
    uint64_t wStartOff_ = 0;
    bool     wValid_ = false;

    // Línea arrastrada entre páginas (ya maquetada pero aún no colocada).
    Line carryLine_;
    bool haveCarry_ = false;

    bool atEnd_ = false;  // La fuente de líneas se agotó.
};

}  // namespace ereader
