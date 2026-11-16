// geometry.hpp — Parámetros de geometría de página (§7.2 Paginator).
//
// Todo en píxeles, como si se dibujara en la pantalla final. Por defecto
// vertical 480×800 (§4), configurable (horizontal 800×480 incluido).
#pragma once

#include <cstdint>

namespace ereader {

struct Geometry {
    int32_t width = 480;         // Ancho de página en px.
    int32_t height = 800;        // Alto de página en px.
    int32_t marginLeft = 24;
    int32_t marginRight = 24;
    int32_t marginTop = 24;
    int32_t marginBottom = 24;
    int32_t lineGap = 2;         // Interlineado extra sobre lineHeight (px).
    int32_t paragraphGap = 10;   // Espacio entre párrafos (px).
    int32_t indent = 28;         // Sangría de primera línea de párrafo (px).

    // Ancho útil para texto.
    int32_t textWidth() const { return width - marginLeft - marginRight; }
    // Alto útil para texto.
    int32_t textHeight() const { return height - marginTop - marginBottom; }
};

}  // namespace ereader
