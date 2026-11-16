// test_tablemetrics — Lectura de la tabla ERFM: anchos, reemplazo, validación.
#include <unity.h>

#include <vector>

#include "ereader/byteio.hpp"
#include "ereader/table_metrics.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static std::vector<uint8_t> buildErfm() {
    std::vector<uint8_t> b(24);
    std::memcpy(b.data(), "ERFM", 4);
    bio::putU16(b.data() + 4, 1);     // version
    bio::putU16(b.data() + 6, 20);    // sizePx
    bio::putU16(b.data() + 8, 24);    // lineHeight
    bio::putU16(b.data() + 10, 18);   // ascent
    bio::putU16(b.data() + 12, 6);    // descent
    bio::putU16(b.data() + 14, 10);   // replAdvance
    bio::putU16(b.data() + 16, 1);    // rangeCount
    bio::putU16(b.data() + 18, 0);
    bio::putU32(b.data() + 20, 0);
    // Rango: 'A'..'C' con avances 11,12,13.
    uint8_t r[6];
    bio::putU32(r, 0x41);
    bio::putU16(r + 4, 3);
    b.insert(b.end(), r, r + 6);
    for (uint16_t a : {11, 12, 13}) {
        uint8_t v[2];
        bio::putU16(v, a);
        b.insert(b.end(), v, v + 2);
    }
    return b;
}

void test_load_and_lookup() {
    auto buf = buildErfm();
    TableMetrics tm;
    TEST_ASSERT_TRUE(isOk(tm.init(buf.data(), buf.size())));
    TEST_ASSERT_EQUAL_UINT16(20, tm.sizePx());
    TEST_ASSERT_EQUAL_INT32(24, tm.lineHeight());
    TEST_ASSERT_EQUAL_INT32(18, tm.ascent());
    TEST_ASSERT_EQUAL_INT32(6, tm.descent());
    TEST_ASSERT_EQUAL_INT32(11, tm.advance('A'));
    TEST_ASSERT_EQUAL_INT32(13, tm.advance('C'));
    TEST_ASSERT_EQUAL_INT32(10, tm.advance('Z'));  // fuera de rango -> reemplazo
    TEST_ASSERT_EQUAL_INT32(10, tm.advance(0x1F600));
}

void test_reject_bad() {
    auto buf = buildErfm();
    buf[0] = 'X';  // magic roto
    TableMetrics tm;
    TEST_ASSERT_FALSE(isOk(tm.init(buf.data(), buf.size())));
    TableMetrics tm2;
    TEST_ASSERT_FALSE(isOk(tm2.init(buf.data(), 10)));  // demasiado corto
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_load_and_lookup);
    RUN_TEST(test_reject_bad);
    return UNITY_END();
}
