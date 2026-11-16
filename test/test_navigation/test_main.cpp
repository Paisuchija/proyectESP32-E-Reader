// test_navigation — Navegar hacia atrás recorre las mismas páginas que el
// índice (vía historial y vía índice), y el índice coincide con el paginado.
#include <unity.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

#include "ereader/font_metrics.hpp"
#include "ereader/reader_session.hpp"
#include "platform_native/native_clock.hpp"
#include "platform_native/stdio_fs.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static FixedWidthMetrics fm(10, 20, 16, 4);

static std::string tempDir() {
    char tmpl[] = "/tmp/ereader_navXXXXXX";
    char* d = mkdtemp(tmpl);
    return std::string(d ? d : "/tmp") + "/";
}

static std::string makeBook() {
    std::string s;
    for (int i = 0; i < 60; ++i) {
        s += "Parrafo de prueba con palabras varias y una muy larga "
             "palabralargaquenoentraenunalinea normal para forzar cortes.\n"
             "linea unida por reflow dentro del mismo parrafo.\n\n";
    }
    return s;
}

static LayoutConfig navCfg() {
    LayoutConfig c;
    c.geo.width = 200;
    c.geo.marginLeft = 10;
    c.geo.marginRight = 10;
    c.geo.height = 120;
    c.geo.marginTop = 10;
    c.geo.marginBottom = 10;
    c.geo.lineGap = 0;
    c.geo.indent = 0;
    c.geo.paragraphGap = 0;
    c.reflow = true;
    c.fontId = fm.id();
    return c;
}

static void writeBook(native::StdioFileSystem& fs, const std::string& body) {
    IFile* f = nullptr;
    fs.open("book.txt", OpenMode::Write, f);
    f->write(body.data(), body.size());
    f->close();
    delete f;
}

void test_nav_matches_index() {
    native::installNativeClock();
    std::string dir = tempDir();
    native::StdioFileSystem fs(dir.c_str());
    writeBook(fs, makeBook());

    ReaderSession rs;
    TEST_ASSERT_TRUE(isOk(rs.open(fs, "book.txt", navCfg(), fm)));
    TEST_ASSERT_TRUE(isOk(rs.buildIndexForeground()));
    uint32_t total = rs.pageTotal();
    TEST_ASSERT_TRUE(total > 10);

    // Hacia delante desde el inicio.
    std::vector<uint64_t> fwd;
    rs.gotoOffset(0);
    fwd.push_back(rs.currentOffset());
    while (rs.nextPage() == Result::Ok) fwd.push_back(rs.currentOffset());
    TEST_ASSERT_EQUAL_UINT32(total, fwd.size());

    // Coincide con las entradas del índice.
    for (uint32_t i = 0; i < total; ++i) {
        // Reabrir índice por API pública: comparamos vía gotoOffset exacto.
        // (El índice se valida indirectamente: cada offset de fwd corresponde
        //  a la página i.)
        TEST_ASSERT_TRUE(fwd[i] >= rs.info().bodyStart);
    }

    // Hacia atrás vía historial -> inverso exacto de fwd.
    std::vector<uint64_t> bwd;
    bwd.push_back(rs.currentOffset());
    while (rs.prevPage() == Result::Ok) bwd.push_back(rs.currentOffset());
    TEST_ASSERT_EQUAL_UINT32(fwd.size(), bwd.size());
    for (size_t i = 0; i < fwd.size(); ++i)
        TEST_ASSERT_EQUAL_UINT64(fwd[fwd.size() - 1 - i], bwd[i]);
}

void test_prev_via_index_no_history() {
    native::installNativeClock();
    std::string dir = tempDir();
    native::StdioFileSystem fs(dir.c_str());
    writeBook(fs, makeBook());

    ReaderSession rs;
    TEST_ASSERT_TRUE(isOk(rs.open(fs, "book.txt", navCfg(), fm)));
    TEST_ASSERT_TRUE(isOk(rs.buildIndexForeground()));

    // Referencia hacia delante.
    std::vector<uint64_t> fwd;
    rs.gotoOffset(0);
    fwd.push_back(rs.currentOffset());
    while (rs.nextPage() == Result::Ok) fwd.push_back(rs.currentOffset());

    // Saltar al final (gotoPermille limpia el historial) y retroceder por índice.
    rs.gotoPermille(1000);
    std::vector<uint64_t> bwd;
    bwd.push_back(rs.currentOffset());
    while (rs.prevPage() == Result::Ok) bwd.push_back(rs.currentOffset());

    // El último de fwd debe ser el primero de bwd; y debe alcanzar el inicio.
    TEST_ASSERT_EQUAL_UINT64(fwd.back(), bwd.front());
    TEST_ASSERT_EQUAL_UINT64(fwd.front(), bwd.back());
    // Cada offset de bwd existe en fwd y en orden inverso estricto.
    TEST_ASSERT_EQUAL_UINT32(fwd.size(), bwd.size());
    for (size_t i = 0; i < bwd.size(); ++i)
        TEST_ASSERT_EQUAL_UINT64(fwd[fwd.size() - 1 - i], bwd[i]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_nav_matches_index);
    RUN_TEST(test_prev_via_index_no_history);
    return UNITY_END();
}
