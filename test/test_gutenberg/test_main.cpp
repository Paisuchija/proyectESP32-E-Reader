// test_gutenberg — Marcadores START/END (THE/THIS) y metadatos.
#include <unity.h>

#include <string>

#include "ereader/gutenberg_info.hpp"
#include "platform_native/mem_file.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static GutenbergInfo detect(const std::string& s) {
    native::MemFile f(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    GutenbergInfo gi;
    detectGutenberg(f, gi);
    return gi;
}

void test_the_variant_and_metadata() {
    std::string s =
        "The Project Gutenberg eBook of Don Quijote\r\n\r\n"
        "Title: Don Quijote\r\n"
        "Author: Miguel de Cervantes\r\n"
        "Language: Spanish\r\n\r\n"
        "*** START OF THE PROJECT GUTENBERG EBOOK DON QUIJOTE ***\r\n"
        "CUERPO primera linea\r\nsegunda\r\n"
        "*** END OF THE PROJECT GUTENBERG EBOOK DON QUIJOTE ***\r\n"
        "Licencia bla bla\r\n";
    GutenbergInfo gi = detect(s);
    TEST_ASSERT_TRUE(gi.isGutenberg);
    TEST_ASSERT_EQUAL_STRING("Don Quijote", gi.title);
    TEST_ASSERT_EQUAL_STRING("Miguel de Cervantes", gi.author);
    TEST_ASSERT_EQUAL_STRING("Spanish", gi.language);
    // bodyStart apunta a "CUERPO"
    TEST_ASSERT_EQUAL_STRING_LEN("CUERPO", s.data() + gi.bodyStart, 6);
    // bodyEnd apunta al inicio de la línea del marcador END
    TEST_ASSERT_EQUAL_STRING_LEN("*** END", s.data() + gi.bodyEnd, 7);
}

void test_this_variant() {
    std::string s =
        "*** START OF THIS PROJECT GUTENBERG EBOOK SOMETHING ***\n"
        "contenido\n"
        "*** END OF THIS PROJECT GUTENBERG EBOOK SOMETHING ***\n";
    GutenbergInfo gi = detect(s);
    TEST_ASSERT_TRUE(gi.isGutenberg);
    TEST_ASSERT_EQUAL_STRING_LEN("contenido", s.data() + gi.bodyStart, 9);
    TEST_ASSERT_EQUAL_STRING_LEN("*** END", s.data() + gi.bodyEnd, 7);
}

void test_case_insensitive() {
    std::string s =
        "*** start of the project gutenberg ebook x ***\nhola\n"
        "*** end of the project gutenberg ebook x ***\n";
    GutenbergInfo gi = detect(s);
    TEST_ASSERT_TRUE(gi.isGutenberg);
    TEST_ASSERT_EQUAL_STRING_LEN("hola", s.data() + gi.bodyStart, 4);
}

void test_not_gutenberg() {
    std::string s = "Un archivo de texto cualquiera\nsin marcadores\n";
    GutenbergInfo gi = detect(s);
    TEST_ASSERT_FALSE(gi.isGutenberg);
    TEST_ASSERT_EQUAL_UINT64(0, gi.bodyStart);
    TEST_ASSERT_EQUAL_UINT64(s.size(), gi.bodyEnd);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_the_variant_and_metadata);
    RUN_TEST(test_this_variant);
    RUN_TEST(test_case_insensitive);
    RUN_TEST(test_not_gutenberg);
    return UNITY_END();
}
