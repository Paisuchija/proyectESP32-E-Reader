// cli_main.cpp — CLI nativa: pagina un TXT real y vuelca páginas (M1).
//
// Uso:
//   native_cli <archivo.txt> [--pages N] [--from K] [--goto PCT]
//              [--reflow on|off] [--w W] [--h H] [--adv A] [--lh H] [--index]
//
// Por defecto usa métricas de ancho fijo (deterministas). Construye la sesión
// de lectura completa (igual que hará el firmware) y usa el PageSink de texto.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "ereader/clock.hpp"
#include "ereader/font_metrics.hpp"
#include "ereader/layout_config.hpp"
#include "ereader/page_sink.hpp"
#include "ereader/reader_session.hpp"
#include "ereader/table_metrics.hpp"
#include "platform_native/native_clock.hpp"
#include "platform_native/stdio_fs.hpp"

using namespace ereader;

// Carga un archivo entero en memoria (para la tabla de métricas .erfm).
static std::vector<uint8_t> loadFile(const char* path) {
    std::vector<uint8_t> buf;
    FILE* f = std::fopen(path, "rb");
    if (!f) return buf;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (n > 0) {
        buf.resize(static_cast<size_t>(n));
        size_t got = std::fread(buf.data(), 1, buf.size(), f);
        buf.resize(got);
    }
    std::fclose(f);
    return buf;
}

static void writeStdout(const char* d, size_t n, void*) { std::fwrite(d, 1, n, stdout); }

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
                     "uso: %s <archivo.txt> [--pages N] [--from K] [--goto PCT]\n"
                     "        [--reflow on|off] [--w W] [--h H] [--adv A] [--lh H] [--index]\n",
                     argv[0]);
        return 2;
    }
    const char* file = argv[1];
    int pages = 5, from = 0, gotoPct = -1;
    bool reflow = true, doIndex = false;
    int w = 480, h = 800, adv = 11, lh = 26;
    const char* fontPath = nullptr;
    for (int i = 2; i < argc; ++i) {
        auto eq = [&](const char* o) { return std::strcmp(argv[i], o) == 0; };
        if (eq("--font") && i + 1 < argc) fontPath = argv[++i];
        else if (eq("--pages") && i + 1 < argc) pages = std::atoi(argv[++i]);
        else if (eq("--from") && i + 1 < argc) from = std::atoi(argv[++i]);
        else if (eq("--goto") && i + 1 < argc) gotoPct = std::atoi(argv[++i]);
        else if (eq("--reflow") && i + 1 < argc) reflow = std::strcmp(argv[++i], "off") != 0;
        else if (eq("--w") && i + 1 < argc) w = std::atoi(argv[++i]);
        else if (eq("--h") && i + 1 < argc) h = std::atoi(argv[++i]);
        else if (eq("--adv") && i + 1 < argc) adv = std::atoi(argv[++i]);
        else if (eq("--lh") && i + 1 < argc) lh = std::atoi(argv[++i]);
        else if (eq("--index")) doIndex = true;
    }

    native::installNativeClock();
    native::StdioFileSystem fs("");  // rutas absolutas/relativas tal cual

    // Métricas: tabla .erfm (Literata) si se indica --font; si no, ancho fijo.
    FixedWidthMetrics fixed(adv, lh, lh * 8 / 10, lh * 2 / 10);
    TableMetrics table;
    std::vector<uint8_t> fontBuf;
    FontMetrics* fm = &fixed;
    if (fontPath) {
        fontBuf = loadFile(fontPath);
        if (isOk(table.init(fontBuf.data(), fontBuf.size()))) {
            fm = &table;
            std::printf("Fuente: tabla %s (size=%u px)\n", fontPath, table.sizePx());
        } else {
            std::fprintf(stderr, "aviso: no se pudo cargar %s; uso ancho fijo\n", fontPath);
        }
    }

    LayoutConfig cfg;
    cfg.geo.width = w;
    cfg.geo.height = h;
    cfg.reflow = reflow;
    cfg.fontId = fm->id();

    ReaderSession rs;
    // En el PC las cachés van a un directorio temporal (en el dispositivo: SD "/sys").
    Result r = rs.open(fs, file, cfg, *fm, "/tmp/ereader_cli_cache");
    if (!isOk(r)) {
        std::fprintf(stderr, "error al abrir '%s': %s\n", file, resultName(r));
        return 1;
    }
    std::printf("Libro: %s\n", rs.info().isGutenberg ? "Project Gutenberg" : "TXT plano");
    if (rs.info().title[0]) std::printf("  Title: %s\n", rs.info().title);
    if (rs.info().author[0]) std::printf("  Author: %s\n", rs.info().author);
    if (rs.info().language[0]) std::printf("  Language: %s\n", rs.info().language);
    std::printf("  cuerpo: [%llu, %llu)  (%llu bytes)\n",
                (unsigned long long)rs.info().bodyStart,
                (unsigned long long)rs.info().bodyEnd,
                (unsigned long long)(rs.info().bodyEnd - rs.info().bodyStart));

    if (doIndex) {
        uint64_t t0 = nowMicros();
        r = rs.buildIndexForeground();
        uint64_t dt = nowMicros() - t0;
        std::printf("Indice: %s, %u paginas en %.1f ms\n", resultName(r),
                    rs.pageTotal(), dt / 1000.0);
    }

    if (gotoPct >= 0) rs.gotoPermille(static_cast<uint32_t>(gotoPct) * 10);
    for (int i = 0; i < from; ++i) rs.nextPage();

    CallbackPageSink sink(writeStdout, nullptr);
    for (int i = 0; i < pages; ++i) {
        const Page& p = rs.currentPage();
        PageHeader hdr;
        hdr.title = rs.info().title[0] ? rs.info().title : file;
        hdr.author = rs.info().author;
        hdr.pageNumber = rs.currentPageNumber();
        hdr.pageTotal = rs.pageTotal();
        hdr.percentX10 = rs.percentX10();
        hdr.indexing = rs.indexing();
        hdr.layoutMicros = rs.lastLayoutMicros();
        sink.render(p, hdr);
        if (rs.nextPage() == Result::End) break;
    }
    return 0;
}
