// test_reflow — Reflow ON/OFF: unión de líneas, párrafos, espacios, NBSP.
#include <unity.h>

#include <string>
#include <vector>

#include "ereader/text_stream.hpp"
#include "ereader/utf8_source.hpp"
#include "platform_native/mem_file.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static std::vector<TokKind> kinds(const std::string& s, bool reflow) {
    native::MemFile f(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    Utf8Source src;
    src.init(&f, 0, s.size());
    ReflowFilter rf;
    rf.init(&src, reflow);
    std::vector<TokKind> out;
    for (;;) {
        Token t;
        rf.next(t);
        out.push_back(t.kind);
        if (t.kind == TokKind::End) break;
    }
    return out;
}

static void assertKinds(const std::vector<TokKind>& got, const std::vector<TokKind>& exp) {
    TEST_ASSERT_EQUAL_UINT32(exp.size(), got.size());
    for (size_t i = 0; i < exp.size() && i < got.size(); ++i)
        TEST_ASSERT_EQUAL_UINT8((uint8_t)exp[i], (uint8_t)got[i]);
}

void test_reflow_on_soft_and_para() {
    using K = TokKind;
    // "a\nb\n\nc": salto simple -> Space; línea en blanco -> ParaBreak.
    assertKinds(kinds("a\nb\n\nc", true),
                {K::Glyph, K::Space, K::Glyph, K::ParaBreak, K::Glyph, K::End});
}

void test_reflow_on_collapse_spaces() {
    using K = TokKind;
    // Espacios múltiples -> un solo Space. Saltos + espacios mezclados -> uno.
    assertKinds(kinds("a   b", true), {K::Glyph, K::Space, K::Glyph, K::End});
    assertKinds(kinds("a \n \n b", true),
                {K::Glyph, K::ParaBreak, K::Glyph, K::End});  // línea en blanco entre medias
    // Espacios iniciales se descartan.
    assertKinds(kinds("   a", true), {K::Glyph, K::End});
}

void test_nbsp_not_collapsed() {
    using K = TokKind;
    std::string s = "a\xC2\xA0";  // 'a' + U+00A0
    s += "b";
    assertKinds(kinds(s, true), {K::Glyph, K::Nbsp, K::Glyph, K::End});
}

void test_reflow_off_hard_breaks() {
    using K = TokKind;
    assertKinds(kinds("a\nb", false), {K::Glyph, K::LineBreak, K::Glyph, K::End});
    // Línea en blanco -> dos LineBreak (línea vacía preservada, poesía).
    assertKinds(kinds("a\n\nb", false),
                {K::Glyph, K::LineBreak, K::LineBreak, K::Glyph, K::End});
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reflow_on_soft_and_para);
    RUN_TEST(test_reflow_on_collapse_spaces);
    RUN_TEST(test_nbsp_not_collapsed);
    RUN_TEST(test_reflow_off_hard_breaks);
    return UNITY_END();
}
