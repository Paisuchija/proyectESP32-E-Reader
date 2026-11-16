// test_realbook — Prueba con un libro real de Gutenberg (pg2000.txt, Don
// Quijote). Se OMITE si el archivo no está (descárgalo con
// `python3 tools/fetch_testbook.py 2000`; no se versiona, ver .gitignore).
//
// Verifica: detección de Gutenberg, construcción de índice y DETERMINISMO sobre
// texto real (la página maquetada desde el offset del índice coincide con la
// paginación continua: su startOffset y el inicio de la siguiente).
#include <unity.h>

#include <string>

#include "ereader/font_metrics.hpp"
#include "ereader/layout.hpp"
#include "ereader/reader_session.hpp"
#include "ereader/text_stream.hpp"
#include "ereader/utf8_source.hpp"
#include "platform_native/native_clock.hpp"
#include "platform_native/stdio_fs.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static FixedWidthMetrics fm(11, 26, 20, 6);

static const char* findBook(native::StdioFileSystem& fs) {
    static const char* candidates[] = {
        "testdata/pg2000.txt", "../testdata/pg2000.txt", "../../testdata/pg2000.txt"};
    for (const char* c : candidates)
        if (fs.exists(c)) return c;
    return nullptr;
}

void test_real_book_index_and_determinism() {
    native::installNativeClock();
    native::StdioFileSystem fs("");
    const char* path = findBook(fs);
    if (!path) {
        TEST_IGNORE_MESSAGE("testdata/pg2000.txt ausente (ejecuta tools/fetch_testbook.py 2000)");
        return;
    }

    LayoutConfig c;
    c.geo.width = 480; c.geo.height = 800;
    c.geo.marginLeft = 24; c.geo.marginRight = 24;
    c.geo.marginTop = 24; c.geo.marginBottom = 24;
    c.reflow = true; c.fontId = fm.id();

    ReaderSession rs;
    TEST_ASSERT_TRUE(isOk(rs.open(fs, path, c, fm, "/tmp/ereader_realbook_cache")));
    TEST_ASSERT_TRUE(rs.info().isGutenberg);
    TEST_ASSERT_EQUAL_STRING("Don Quijote", rs.info().title);
    TEST_ASSERT_TRUE(isOk(rs.buildIndexForeground()));
    uint32_t total = rs.pageTotal();
    TEST_ASSERT_TRUE(total > 1000);  // ~2300 páginas con esta geometría

    // Determinismo sobre el libro real: paginando en continuo, cada startOffset
    // debe coincidir con avanzar página a página desde el inicio.
    IFile* f = nullptr;
    fs.open(path, OpenMode::Read, f);
    GutenbergInfo gi = rs.info();
    Utf8Source s; s.init(f, gi.bodyStart, gi.bodyEnd);
    ReflowFilter rf; rf.init(&s, true);
    LayoutEngine le; le.init(&rf, c.geo, &fm, true);

    uint32_t idx = 0, checked = 0;
    for (;;) {
        Page p;
        if (le.nextPage(p) == Result::End) break;
        if (idx % 200 == 0) {
            // Repaginar SOLO esta página desde su propio offset y comparar.
            native::StdioFileSystem fs2("");
            IFile* f2 = nullptr; fs2.open(path, OpenMode::Read, f2);
            Utf8Source s2; s2.init(f2, gi.bodyStart, gi.bodyEnd); s2.seekTo(p.startOffset);
            ReflowFilter rf2; rf2.init(&s2, true);
            LayoutEngine le2; le2.init(&rf2, c.geo, &fm, p.startsParagraph);
            Page p2; le2.nextPage(p2);
            TEST_ASSERT_EQUAL_UINT64(p.startOffset, p2.startOffset);
            TEST_ASSERT_EQUAL_UINT16(p.lineCount, p2.lineCount);
            TEST_ASSERT_EQUAL_UINT64(le.nextPageOffset(), le2.nextPageOffset());
            f2->close(); delete f2;
            ++checked;
        }
        ++idx;
    }
    f->close(); delete f;
    TEST_ASSERT_TRUE(checked > 5);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_real_book_index_and_determinism);
    return UNITY_END();
}
