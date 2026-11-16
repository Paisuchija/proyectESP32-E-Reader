// test_paginator — DETERMINISMO (§7.2): paginar desde el offset de cualquier
// página produce exactamente las mismas páginas que paginar en continuo.
#include <unity.h>

#include <string>
#include <vector>

#include "ereader/font_metrics.hpp"
#include "ereader/layout.hpp"
#include "ereader/text_stream.hpp"
#include "ereader/utf8_source.hpp"
#include "platform_native/mem_file.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static FixedWidthMetrics fm(10, 20, 16, 4);

struct Snap {
    uint64_t startOffset;
    bool startsPara;
    std::vector<uint64_t> lineOffsets;
    std::vector<std::vector<uint32_t>> glyphs;
    bool operator==(const Snap& o) const {
        return startOffset == o.startOffset && startsPara == o.startsPara &&
               lineOffsets == o.lineOffsets && glyphs == o.glyphs;
    }
};

static Snap snap(const Page& p) {
    Snap s;
    s.startOffset = p.startOffset;
    s.startsPara = p.startsParagraph;
    for (int i = 0; i < p.lineCount; ++i) {
        s.lineOffsets.push_back(p.lines[i].startOffset);
        std::vector<uint32_t> g;
        for (int j = 0; j < p.lines[i].count; ++j) g.push_back(p.lines[i].glyphs[j].cp);
        s.glyphs.push_back(g);
    }
    return s;
}

static Geometry smallGeo() {
    Geometry g;
    g.width = 130;  // textWidth 120 -> 12 glifos
    g.marginLeft = 5;
    g.marginRight = 5;
    g.height = 85;  // ~4 líneas de 20
    g.marginTop = 5;
    g.marginBottom = 5;
    g.lineGap = 0;
    g.indent = 0;
    g.paragraphGap = 0;
    return g;
}

void test_determinism() {
    std::string body;
    for (int i = 0; i < 40; ++i) {
        body += "Parrafo numero con varias palabras cortas y una larga "
                "superlargapalabraquenoentra en la linea normal.\n"
                "segunda linea del parrafo unida por reflow suave.\n\n";
    }
    const uint8_t* data = reinterpret_cast<const uint8_t*>(body.data());
    uint64_t size = body.size();
    Geometry geo = smallGeo();

    // Continuo.
    native::MemFile f0(data, size);
    Utf8Source s0;
    s0.init(&f0, 0, size);
    ReflowFilter rf0;
    rf0.init(&s0, true);
    LayoutEngine le0;
    le0.init(&rf0, geo, &fm, true);
    std::vector<Snap> pages;
    std::vector<bool> paras;
    for (;;) {
        Page p;
        if (le0.nextPage(p) == Result::End) break;
        pages.push_back(snap(p));
        paras.push_back(p.startsParagraph);
    }
    TEST_ASSERT_TRUE(pages.size() > 10);

    // Reinicio desde cada página.
    for (size_t k = 0; k < pages.size(); ++k) {
        native::MemFile f(data, size);
        Utf8Source s;
        s.init(&f, 0, size);
        s.seekTo(pages[k].startOffset);
        ReflowFilter rf;
        rf.init(&s, true);
        LayoutEngine le;
        le.init(&rf, geo, &fm, paras[k]);
        Page p;
        Result r = le.nextPage(p);
        TEST_ASSERT_TRUE(isOk(r));
        TEST_ASSERT_TRUE(snap(p) == pages[k]);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_determinism);
    return UNITY_END();
}
