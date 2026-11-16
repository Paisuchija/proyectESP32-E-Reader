// test_progress — Escritura atómica y recuperación desde .bak / CRC.
#include <unity.h>

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "ereader/progress.hpp"
#include "platform_native/stdio_fs.hpp"

using namespace ereader;

void setUp() {}
void tearDown() {}

static std::string tempDir() {
    char tmpl[] = "/tmp/ereader_prgXXXXXX";
    char* d = mkdtemp(tmpl);
    return std::string(d ? d : "/tmp") + "/";
}

static const BookId kId{0x1234abcd, 123456};

static Progress mk(uint64_t off) {
    Progress p;
    p.byteOffset = off;
    p.layoutHash = 0xAA55;
    p.timestampMs = off;
    return p;
}

void test_roundtrip() {
    std::string dir = tempDir();
    native::StdioFileSystem fs(dir.c_str());
    TEST_ASSERT_TRUE(isOk(ProgressStore::save(fs, "p.prg", kId, mk(111))));
    Progress out;
    TEST_ASSERT_TRUE(isOk(ProgressStore::load(fs, "p.prg", kId, out)));
    TEST_ASSERT_EQUAL_UINT64(111, out.byteOffset);
    // Un BookId distinto no debe cargar.
    BookId other{0, 0};
    TEST_ASSERT_FALSE(isOk(ProgressStore::load(fs, "p.prg", other, out)));
}

void test_recover_from_bak_on_crash_between_tmp_and_rename() {
    std::string dir = tempDir();
    native::StdioFileSystem fs(dir.c_str());
    ProgressStore::save(fs, "p.prg", kId, mk(111));  // path=111
    ProgressStore::save(fs, "p.prg", kId, mk(222));  // path=222, bak=111
    ProgressStore::save(fs, "p.prg", kId, mk(333));  // path=333, bak=222
    // Simular corte justo antes del rename final: path.tmp=333 (nuevo), bak=222,
    // path ausente.
    TEST_ASSERT_TRUE(isOk(fs.rename("p.prg", "p.prg.tmp")));
    Progress out;
    TEST_ASSERT_TRUE(isOk(ProgressStore::load(fs, "p.prg", kId, out)));
    TEST_ASSERT_EQUAL_UINT64(222, out.byteOffset);  // recuperado del .bak
}

void test_recover_from_bak_on_corrupt_primary() {
    std::string dir = tempDir();
    native::StdioFileSystem fs(dir.c_str());
    ProgressStore::save(fs, "p.prg", kId, mk(111));  // path=111
    ProgressStore::save(fs, "p.prg", kId, mk(222));  // path=222, bak=111
    // Corromper el primario.
    IFile* f = nullptr;
    fs.open("p.prg", OpenMode::Update, f);
    f->seek(20, Seek::Set);  // dentro de byteOffset
    uint8_t junk = 0xFF;
    f->write(&junk, 1);
    f->flush(); f->close(); delete f;
    Progress out;
    TEST_ASSERT_TRUE(isOk(ProgressStore::load(fs, "p.prg", kId, out)));
    TEST_ASSERT_EQUAL_UINT64(111, out.byteOffset);  // del .bak válido
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip);
    RUN_TEST(test_recover_from_bak_on_crash_between_tmp_and_rename);
    RUN_TEST(test_recover_from_bak_on_corrupt_primary);
    return UNITY_END();
}
