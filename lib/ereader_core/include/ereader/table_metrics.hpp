// table_metrics.hpp — Métricas tipográficas desde una tabla binaria (§7.2).
//
// La tabla la genera tools/gen_metrics.py a partir de una fuente TTF con
// licencia OFL (Literata), para 3 tamaños. Aquí solo se LEE; no se asigna
// memoria por glifo. El buffer lo aporta el llamante (archivo en SD o array
// embebido); TableMetrics no toma su propiedad.
//
// Formato ERFM (little-endian). Cabecera 24 bytes:
//   0 magic "ERFM" | 4 version u16 | 6 sizePx u16 | 8 lineHeight u16 |
//   10 ascent u16 | 12 descent u16 | 14 replAdvance u16 | 16 rangeCount u16 |
//   18 reserved u16 | 20 reserved u32
// Luego, por cada rango (contiguo): firstCp u32, count u16, count×u16 advance.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ereader/font_metrics.hpp"
#include "ereader/result.hpp"

namespace ereader {

class TableMetrics : public FontMetrics {
public:
    static constexpr int kMaxRanges = 16;

    // Inicializa sobre `data` (formato ERFM). El buffer debe sobrevivir al uso.
    Result init(const uint8_t* data, size_t len);

    int32_t advance(uint32_t cp) const override;
    int32_t lineHeight() const override { return lineHeight_; }
    int32_t ascent() const override { return ascent_; }
    int32_t descent() const override { return descent_; }
    uint32_t id() const override { return id_; }

    uint16_t sizePx() const { return sizePx_; }

private:
    struct Range {
        uint32_t first;
        uint16_t count;
        const uint8_t* adv;  // apunta a count×u16 dentro de `data`
    };

    const uint8_t* data_ = nullptr;
    int32_t lineHeight_ = 0, ascent_ = 0, descent_ = 0;
    int32_t replAdvance_ = 0;
    uint16_t sizePx_ = 0;
    uint32_t id_ = 0;
    Range ranges_[kMaxRanges];
    int rangeCount_ = 0;
};

}  // namespace ereader
