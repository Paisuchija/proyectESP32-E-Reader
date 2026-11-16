// page_sink.cpp — Sink de texto por callback.
#include "ereader/page_sink.hpp"

#include <cstdio>
#include <cstring>

#include "ereader/utf8_encode.hpp"

namespace ereader {

void CallbackPageSink::writeStr(const char* s) { fn_(s, std::strlen(s), user_); }

void CallbackPageSink::render(const Page& page, const PageHeader& header) {
    char buf[128];
    // Cabecera: título — autor
    writeStr("\n====================================================\n");
    std::snprintf(buf, sizeof(buf), " %s", header.title && header.title[0] ? header.title : "(sin titulo)");
    writeStr(buf);
    if (header.author && header.author[0]) {
        std::snprintf(buf, sizeof(buf), " — %s", header.author);
        writeStr(buf);
    }
    writeStr("\n");
    // Línea de estado: página X/Y o ~% + tiempos.
    if (header.indexing || header.pageTotal == 0) {
        std::snprintf(buf, sizeof(buf), " pag %u (~%u.%u%%, indexando)  layout=%uus\n",
                      header.pageNumber, header.percentX10 / 10, header.percentX10 % 10,
                      header.layoutMicros);
    } else {
        std::snprintf(buf, sizeof(buf), " pag %u/%u (%u.%u%%)  layout=%uus\n",
                      header.pageNumber, header.pageTotal,
                      header.percentX10 / 10, header.percentX10 % 10, header.layoutMicros);
    }
    writeStr(buf);
    writeStr("----------------------------------------------------\n");

    // Cuerpo: una línea de texto por cada línea maquetada (tal cual la cortó el
    // layout). Se reconstruye el texto desde los glifos en orden.
    for (int i = 0; i < page.lineCount; ++i) {
        const Line& ln = page.lines[i];
        char line[cfg::kMaxGlyphsPerLine * 4 + 8];
        size_t n = 0;
        for (int j = 0; j < ln.count && n + 4 < sizeof(line); ++j) {
            n += utf8Encode(ln.glyphs[j].cp, line + n);
        }
        line[n++] = '\n';
        fn_(line, n, user_);
    }
}

}  // namespace ereader
