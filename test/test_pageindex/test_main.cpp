// test_pageindex — CRC corrupto -> se rechaza; layout_hash distinto -> rechazo;
// indexación interrumpida y reanudada -> mismo resultado.
#include <unity.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

#include "ereader/book_id.hpp"
#include "ereader/font_metrics.hpp"
#include "ereader/layout.hpp"
#include "ereader/layout_config.hpp"
#include "ereader/page_index.hpp"
#include "ereader/text_stream.hpp"
#include "ereader/utf8_source.hpp"
#include "platform_native/stdio_fs.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static FixedWidthMetrics fm(10, 20, 16, 4);

static std::string tempDir() {
    char tmpl[] = "/tmp/ereader_idxXXXXXX";
    char* d = mkdtemp(tmpl);
    return std::string(d ? d : "/tmp") + "/";
}

static Geometry geo() {
    Geometry g;
    g.width = 160; g.marginLeft = 10; g.marginRight = 10;
    g.height = 110; g.marginTop = 10; g.marginBottom = 10;
    g.lineGap = 0; g.indent = 0; g.paragraphGap = 0;
    return g;
}

// Pagina todo el libro y devuelve (offset,flags) de cada página.
struct Ent { uint64_t off; uint16_t fl; };
static std::vector<Ent> paginateAll(native::StdioFileSystem& fs, uint64_t size) {
    IFile* f = nullptr;
    fs.open("book.txt", OpenMode::Read, f);
    Utf8Source s; s.init(f, 0, size);
    ReflowFilter rf; rf.init(&s, true);
    LayoutEngine le; le.init(&rf, geo(), &fm, true);
    std::vector<Ent> v;
    for (;;) {
        Page p;
        if (le.nextPage(p) == Result::End) break;
        v.push_back({p.startOffset, (uint16_t)(p.startsParagraph ? kPageFlagParaStart : 0)});
    }
    f->close(); delete f;
    return v;
}

static std::string makeBook() {
    std::string s;
    for (int i = 0; i < 50; ++i)
        s += "Linea de un parrafo con varias palabras para generar paginas.\notra.\n\n";
    return s;
}

static void setup(native::StdioFileSystem& fs, std::string& dir, uint64_t& size,
                  BookId& id, uint32_t& lh) {
    dir = tempDir();
    fs = native::StdioFileSystem(dir.c_str());
    std::string body = makeBook();
    size = body.size();
    IFile* f = nullptr; fs.open("book.txt", OpenMode::Write, f);
    f->write(body.data(), body.size()); f->close(); delete f;
    fs.open("book.txt", OpenMode::Read, f);
    computeBookId(*f, id); f->close(); delete f;
    LayoutConfig c; c.geo = geo(); c.reflow = true; c.fontId = fm.id();
    lh = c.hash();
}

void test_complete_and_crc_corruption() {
    native::StdioFileSystem fs(""); std::string dir; uint64_t size; BookId id; uint32_t lh;
    setup(fs, dir, size, id, lh);
    auto ref = paginateAll(fs, size);
    TEST_ASSERT_TRUE(ref.size() > 5);

    // Construir completo.
    {
        PageIndex idx;
        uint32_t rp; uint64_t ro; bool rpar;
        TEST_ASSERT_TRUE(isOk(idx.createOrResume(fs, "idx.bin", id, lh, 0, size, rp, ro, rpar)));
        for (auto& e : ref) idx.appendPage(e.off, e.fl);
        TEST_ASSERT_TRUE(isOk(idx.finalize()));
    }
    {
        PageIndex idx;
        idx.openRead(fs, "idx.bin", id, lh);
        TEST_ASSERT_EQUAL_INT((int)PageIndex::State::Complete, (int)idx.state());
        TEST_ASSERT_EQUAL_UINT32(ref.size(), idx.pageCount());
        for (uint32_t i = 0; i < idx.pageCount(); ++i) {
            uint64_t o; uint16_t fl; idx.getEntry(i, o, fl);
            TEST_ASSERT_EQUAL_UINT64(ref[i].off, o);
        }
    }
    // Corromper un byte de las entradas -> CRC falla -> Mismatch.
    {
        IFile* f = nullptr; fs.open("idx.bin", OpenMode::Update, f);
        f->seek(50, Seek::Set);
        uint8_t b; size_t g; f->read(&b, 1, g);
        f->seek(50, Seek::Set);
        uint8_t nb = b ^ 0xFF; f->write(&nb, 1); f->flush(); f->close(); delete f;
    }
    {
        PageIndex idx;
        idx.openRead(fs, "idx.bin", id, lh);
        TEST_ASSERT_EQUAL_INT((int)PageIndex::State::Mismatch, (int)idx.state());
    }
}

void test_layout_hash_mismatch() {
    native::StdioFileSystem fs(""); std::string dir; uint64_t size; BookId id; uint32_t lh;
    setup(fs, dir, size, id, lh);
    auto ref = paginateAll(fs, size);
    PageIndex idx;
    uint32_t rp; uint64_t ro; bool rpar;
    idx.createOrResume(fs, "idx.bin", id, lh, 0, size, rp, ro, rpar);
    for (auto& e : ref) idx.appendPage(e.off, e.fl);
    idx.finalize();
    idx.close();
    // Abrir con otro layout_hash -> Mismatch.
    PageIndex idx2;
    idx2.openRead(fs, "idx.bin", id, lh ^ 0x12345678u);
    TEST_ASSERT_EQUAL_INT((int)PageIndex::State::Mismatch, (int)idx2.state());
}

void test_interrupted_resume() {
    native::StdioFileSystem fs(""); std::string dir; uint64_t size; BookId id; uint32_t lh;
    setup(fs, dir, size, id, lh);
    auto ref = paginateAll(fs, size);
    uint32_t K = ref.size() / 2;

    // Parcial: primeras K páginas, flushHeader (sin finalize).
    {
        PageIndex idx;
        uint32_t rp; uint64_t ro; bool rpar;
        idx.createOrResume(fs, "idx.bin", id, lh, 0, size, rp, ro, rpar);
        for (uint32_t i = 0; i < K; ++i) idx.appendPage(ref[i].off, ref[i].fl);
        idx.flushHeader();
        idx.close();
    }
    {
        PageIndex idx;
        idx.openRead(fs, "idx.bin", id, lh);
        TEST_ASSERT_EQUAL_INT((int)PageIndex::State::Partial, (int)idx.state());
        TEST_ASSERT_EQUAL_UINT32(K, idx.pageCount());
    }
    // Reanudar: createOrResume indica desde dónde; repaginar y completar.
    {
        PageIndex idx;
        uint32_t rp; uint64_t ro; bool rpar;
        idx.createOrResume(fs, "idx.bin", id, lh, 0, size, rp, ro, rpar);
        TEST_ASSERT_EQUAL_UINT32(K - 1, rp);              // reescribe la última
        TEST_ASSERT_EQUAL_UINT64(ref[K - 1].off, ro);    // desde su offset
        // Repaginar desde ro con el flag correcto y añadir hasta el final.
        IFile* f = nullptr; fs.open("book.txt", OpenMode::Read, f);
        Utf8Source s; s.init(f, 0, size); s.seekTo(ro);
        ReflowFilter rf; rf.init(&s, true);
        LayoutEngine le; le.init(&rf, geo(), &fm, rpar);
        for (;;) {
            Page p;
            if (le.nextPage(p) == Result::End) break;
            idx.appendPage(p.startOffset, p.startsParagraph ? kPageFlagParaStart : 0);
        }
        idx.finalize();
        f->close(); delete f;
    }
    // Resultado idéntico al completo de una pasada.
    {
        PageIndex idx;
        idx.openRead(fs, "idx.bin", id, lh);
        TEST_ASSERT_EQUAL_INT((int)PageIndex::State::Complete, (int)idx.state());
        TEST_ASSERT_EQUAL_UINT32(ref.size(), idx.pageCount());
        for (uint32_t i = 0; i < idx.pageCount(); ++i) {
            uint64_t o; uint16_t fl; idx.getEntry(i, o, fl);
            TEST_ASSERT_EQUAL_UINT64(ref[i].off, o);
        }
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_complete_and_crc_corruption);
    RUN_TEST(test_layout_hash_mismatch);
    RUN_TEST(test_interrupted_resume);
    return UNITY_END();
}
