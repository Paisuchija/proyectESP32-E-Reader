// layout_config.hpp — Configuración que determina la paginación.
//
// Agrupa todo lo que, si cambia, cambia el resultado de paginar: geometría,
// reflow y la identidad de la fuente. Su hash (layout_hash) etiqueta el índice
// de páginas; si no coincide, se usa o crea otro índice (§7.2).
#pragma once

#include <cstdint>

#include "ereader/byteio.hpp"
#include "ereader/crc32.hpp"
#include "ereader/geometry.hpp"

namespace ereader {

struct LayoutConfig {
    Geometry geo{};
    bool reflow = true;
    uint32_t fontId = 0;  // FontMetrics::id().

    // Hash estable de todos los parámetros que afectan a la paginación.
    uint32_t hash() const {
        uint8_t b[64];
        size_t n = 0;
        auto u32 = [&](int32_t v) { bio::putU32(b + n, static_cast<uint32_t>(v)); n += 4; };
        u32(geo.width); u32(geo.height);
        u32(geo.marginLeft); u32(geo.marginRight);
        u32(geo.marginTop); u32(geo.marginBottom);
        u32(geo.lineGap); u32(geo.paragraphGap); u32(geo.indent);
        b[n++] = reflow ? 1 : 0;
        bio::putU32(b + n, fontId); n += 4;
        return Crc32::compute(b, n);
    }
};

}  // namespace ereader
