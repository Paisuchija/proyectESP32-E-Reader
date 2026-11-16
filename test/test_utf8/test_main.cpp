// test_utf8 — UTF-8 con estado: bordes de bloque, inválidos, overlong, BOM, CRLF.
#include <unity.h>

#include <vector>

#include "ereader/utf8_decoder.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static std::vector<uint32_t> decWhole(const std::vector<uint8_t>& b) {
    Utf8Decoder d;
    std::vector<uint32_t> out(b.size() + 4);
    size_t n = 0;
    d.decode(b.data(), b.size(), 0, out.data(), nullptr, out.size(), n);
    out.resize(n);
    uint32_t f[4];
    size_t fn = d.finish(f, nullptr, 4);
    for (size_t i = 0; i < fn; ++i) out.push_back(f[i]);
    return out;
}

// Decodifica byte a byte (peor caso de bordes de bloque).
static std::vector<uint32_t> decByByte(const std::vector<uint8_t>& b) {
    Utf8Decoder d;
    std::vector<uint32_t> out;
    for (uint8_t x : b) {
        uint32_t o[4];
        size_t n = 0;
        d.decode(&x, 1, 0, o, nullptr, 4, n);
        for (size_t i = 0; i < n; ++i) out.push_back(o[i]);
    }
    uint32_t f[4];
    size_t fn = d.finish(f, nullptr, 4);
    for (size_t i = 0; i < fn; ++i) out.push_back(f[i]);
    return out;
}

static void assertEq(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    TEST_ASSERT_EQUAL_UINT32(a.size(), b.size());
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
        TEST_ASSERT_EQUAL_UINT32(a[i], b[i]);
}

// á=C3A1 ñ=C3B1 ¿=C2BF ¡=C2A1 —=E28094 "comillas" … emoji=F09F9880
void test_multibyte_all_boundaries() {
    std::vector<uint8_t> s = {'A', 0xC3, 0xA1, 0xC3, 0xB1, 0xC2, 0xBF, 0xC2, 0xA1,
                              0xE2, 0x80, 0x94, 0xE2, 0x80, 0xA6, 0xF0, 0x9F, 0x98, 0x80, 'Z'};
    std::vector<uint32_t> exp = {'A', 0xE1, 0xF1, 0xBF, 0xA1, 0x2014, 0x2026, 0x1F600, 'Z'};
    assertEq(decWhole(s), exp);
    assertEq(decByByte(s), exp);  // partido en todas las posiciones
}

void test_invalid_and_overlong() {
    // C0 80 = overlong de U+0000: secuencia COMPLETA pero overlong -> un U+FFFD.
    assertEq(decWhole({0xC0, 0x80}), {0xFFFD});
    // C0 'A' = lead de 2 bytes interrumpido por ASCII -> U+FFFD + 'A' (reproceso).
    assertEq(decWhole({0xC0, 'A'}), {0xFFFD, 'A'});
    // E0 80 80 = overlong -> U+FFFD (+ reprocesos).
    auto r = decWhole({0xE0, 0x80, 0x80});
    TEST_ASSERT_TRUE(r.size() >= 1 && r[0] == 0xFFFD);
    // Continuación suelta.
    assertEq(decWhole({0x80, 'z'}), {0xFFFD, 'z'});
    // Truncado + ASCII: no se pierde la 'A'.
    assertEq(decWhole({0xC3, 'A'}), {0xFFFD, 'A'});
    assertEq(decByByte({0xC3, 'A'}), {0xFFFD, 'A'});
    // Surrogate codificado (ED A0 80 = U+D800) -> U+FFFD.
    auto s = decWhole({0xED, 0xA0, 0x80});
    TEST_ASSERT_TRUE(s.size() >= 1 && s[0] == 0xFFFD);
    // Truncado al EOF.
    assertEq(decWhole({'x', 0xC3}), {'x', 0xFFFD});
}

void test_bom() {
    assertEq(decWhole({0xEF, 0xBB, 0xBF, 'A'}), {'A'});           // BOM inicial saltado
    assertEq(decWhole({'A', 0xEF, 0xBB, 0xBF}), {'A', 0xFEFF});   // BOM en medio se conserva
    assertEq(decByByte({0xEF, 0xBB, 0xBF, 'A'}), {'A'});          // BOM partido
}

void test_crlf() {
    assertEq(decWhole({'a', 0x0D, 0x0A, 'b', 0x0D, 'c', 0x0A, 'd'}),
             {'a', 0x0A, 'b', 0x0A, 'c', 0x0A, 'd'});
    assertEq(decByByte({'a', 0x0D, 0x0A, 'b'}), {'a', 0x0A, 'b'});  // CRLF partido
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_multibyte_all_boundaries);
    RUN_TEST(test_invalid_and_overlong);
    RUN_TEST(test_bom);
    RUN_TEST(test_crlf);
    return UNITY_END();
}
