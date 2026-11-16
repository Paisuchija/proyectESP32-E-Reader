// test_linebreak — Corte voraz por ancho: ajuste exacto, palabra larga,
// espacios múltiples, NBSP sin corte.
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

// Métricas fijas: cada glifo 10 px, altura 20.
static FixedWidthMetrics fm(10, 20, 16, 4);

static std::string lineText(const Line& ln) {
    std::string o;
    for (int i = 0; i < ln.count; ++i) {
        uint32_t c = ln.glyphs[i].cp;
        o += (c < 128) ? static_cast<char>(c) : '?';  // NBSP/no-ASCII -> '?'
    }
    return o;
}

// Parte `s` en líneas con un ancho de texto de `glyphs` glifos (10 px c/u).
static std::vector<std::string> breakLines(const std::string& s, int glyphs) {
    native::MemFile f(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    Utf8Source src;
    src.init(&f, 0, s.size());
    ReflowFilter rf;
    rf.init(&src, true);
    Geometry geo;
    geo.width = glyphs * 10;
    geo.marginLeft = 0;
    geo.marginRight = 0;
    geo.indent = 0;
    LayoutEngine le;
    le.init(&rf, geo, &fm, false);  // sin sangría
    std::vector<std::string> out;
    for (;;) {
        Line ln;
        if (le.nextLine(ln) == Result::End) break;
        out.push_back(lineText(ln));
    }
    return out;
}

void test_exact_fit() {
    auto v = breakLines("abcde fghij", 5);  // dos palabras de 5 en ancho 5
    TEST_ASSERT_EQUAL_UINT32(2, v.size());
    TEST_ASSERT_EQUAL_STRING("abcde", v[0].c_str());
    TEST_ASSERT_EQUAL_STRING("fghij", v[1].c_str());
}

void test_word_longer_than_line() {
    auto v = breakLines("abcdefghijkl", 5);  // 12 chars, ancho 5 -> 5/5/2
    TEST_ASSERT_EQUAL_UINT32(3, v.size());
    TEST_ASSERT_EQUAL_STRING("abcde", v[0].c_str());
    TEST_ASSERT_EQUAL_STRING("fghij", v[1].c_str());
    TEST_ASSERT_EQUAL_STRING("kl", v[2].c_str());
}

void test_collapse_spaces() {
    auto v = breakLines("a     b", 10);  // espacios colapsados
    TEST_ASSERT_EQUAL_UINT32(1, v.size());
    TEST_ASSERT_EQUAL_STRING("a b", v[0].c_str());
}

void test_nbsp_no_break() {
    // Con espacio normal, "aa a" en ancho 3 se parte en "aa" / "a".
    auto sp = breakLines("aa a", 3);
    TEST_ASSERT_EQUAL_UINT32(2, sp.size());
    TEST_ASSERT_EQUAL_STRING("aa", sp[0].c_str());
    TEST_ASSERT_EQUAL_STRING("a", sp[1].c_str());
    // Con NBSP, "aa a" NO corta en el NBSP: la primera línea lleva 3 glifos.
    std::string s = "aa\xC2\xA0";
    s += "a";
    auto nb = breakLines(s, 3);
    TEST_ASSERT_EQUAL_UINT32(2, nb.size());
    TEST_ASSERT_EQUAL_UINT32(3, nb[0].size());  // aa + NBSP juntos
    TEST_ASSERT_EQUAL_STRING("a", nb[1].c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_exact_fit);
    RUN_TEST(test_word_longer_than_line);
    RUN_TEST(test_collapse_spaces);
    RUN_TEST(test_nbsp_no_break);
    return UNITY_END();
}
