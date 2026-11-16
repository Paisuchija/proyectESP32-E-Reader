// gutenberg_info.cpp — Implementación de la detección de Gutenberg.
#include "ereader/gutenberg_info.hpp"

#include <cstring>

#include "ereader/config.hpp"

namespace ereader {

namespace {

char upperAscii(char c) {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

// ¿`hay` (longitud hayLen) contiene la subcadena ASCII `needle`, ignorando
// mayúsculas? Búsqueda ingenua: las líneas son cortas.
bool containsCI(const char* hay, size_t hayLen, const char* needle) {
    size_t nlen = std::strlen(needle);
    if (nlen == 0 || nlen > hayLen) return false;
    for (size_t i = 0; i + nlen <= hayLen; ++i) {
        size_t j = 0;
        while (j < nlen && upperAscii(hay[i + j]) == needle[j]) ++j;
        if (j == nlen) return true;
    }
    return false;
}

// Copia el valor de una cabecera "Campo: valor" a `dst`, recortando espacios.
// `line` apunta justo tras los dos puntos.
void copyTrimmed(const char* line, size_t len, char* dst, size_t dstCap) {
    // Recortar espacios iniciales.
    size_t s = 0;
    while (s < len && (line[s] == ' ' || line[s] == '\t')) ++s;
    size_t e = len;
    while (e > s && (line[e - 1] == ' ' || line[e - 1] == '\t' || line[e - 1] == '\r')) --e;
    size_t n = e - s;
    if (n >= dstCap) n = dstCap - 1;
    std::memcpy(dst, line + s, n);
    dst[n] = '\0';
}

// ¿`line` empieza (ignorando mayúsculas) por el prefijo `pref`?
bool startsWithCI(const char* line, size_t len, const char* pref) {
    size_t p = std::strlen(pref);
    if (len < p) return false;
    for (size_t i = 0; i < p; ++i) {
        if (upperAscii(line[i]) != pref[i]) return false;
    }
    return true;
}

}  // namespace

Result detectGutenberg(IFile& file, GutenbergInfo& out) {
    out = GutenbergInfo{};
    uint64_t fileSize = file.size();
    out.bodyStart = 0;
    out.bodyEnd = fileSize;

    // --- Fase 1: escanear la cabecera buscando START y metadatos. ---
    uint64_t headerEnd = fileSize < cfg::kGutenHeaderWindow ? fileSize
                                                            : cfg::kGutenHeaderWindow;
    Result r = file.seek(0, Seek::Set);
    if (!isOk(r)) return Result::ErrIO;

    uint8_t block[cfg::kReadBlockBytes];
    char line[cfg::kScanLineMax];
    size_t lineLen = 0;
    uint64_t lineStart = 0;
    uint64_t pos = 0;
    bool startFound = false;

    auto processHeaderLine = [&](const char* l, size_t len, uint64_t lStart,
                                 uint64_t afterNewline) -> bool {
        if (containsCI(l, len, "PROJECT GUTENBERG EBOOK") &&
            containsCI(l, len, "*** START OF T")) {
            out.isGutenberg = true;
            out.bodyStart = afterNewline;  // El cuerpo empieza tras esta línea.
            startFound = true;
            return true;  // Parar el escaneo de cabecera.
        }
        (void)lStart;
        // Metadatos (solo antes del START).
        if (startsWithCI(l, len, "TITLE:") && out.title[0] == '\0') {
            copyTrimmed(l + 6, len - 6, out.title, kGutenMetaLen);
        } else if (startsWithCI(l, len, "AUTHOR:") && out.author[0] == '\0') {
            copyTrimmed(l + 7, len - 7, out.author, kGutenMetaLen);
        } else if (startsWithCI(l, len, "LANGUAGE:") && out.language[0] == '\0') {
            copyTrimmed(l + 9, len - 9, out.language, kGutenMetaLen);
        }
        return false;
    };

    while (pos < headerEnd && !startFound) {
        uint64_t remaining = headerEnd - pos;
        size_t want = static_cast<size_t>(
            remaining < cfg::kReadBlockBytes ? remaining : cfg::kReadBlockBytes);
        size_t got = 0;
        r = file.read(block, want, got);
        if (!isOk(r)) return Result::ErrIO;
        if (got == 0) break;
        for (size_t i = 0; i < got && !startFound; ++i) {
            uint8_t b = block[i];
            uint64_t byteOff = pos + i;
            if (b == '\n') {
                if (processHeaderLine(line, lineLen, lineStart, byteOff + 1)) break;
                lineLen = 0;
                lineStart = byteOff + 1;
            } else {
                if (lineLen < cfg::kScanLineMax) line[lineLen++] = static_cast<char>(b);
            }
        }
        pos += got;
    }
    // Si quedó una línea sin \n dentro de la ventana (p. ej. START al final).
    if (!startFound && lineLen > 0) {
        processHeaderLine(line, lineLen, lineStart, pos);
    }

    if (!out.isGutenberg) {
        // No es Gutenberg: cuerpo = archivo entero.
        out.bodyStart = 0;
        out.bodyEnd = fileSize;
        return Result::Ok;
    }

    // --- Fase 2: buscar END en una ventana de cola acotada. ---
    uint64_t tailStart = out.bodyStart;
    if (fileSize > cfg::kGutenTailWindow &&
        fileSize - cfg::kGutenTailWindow > out.bodyStart) {
        tailStart = fileSize - cfg::kGutenTailWindow;
    }
    r = file.seek(static_cast<int64_t>(tailStart), Seek::Set);
    if (!isOk(r)) return Result::ErrIO;

    lineLen = 0;
    lineStart = tailStart;
    pos = tailStart;
    bool endFound = false;
    while (pos < fileSize && !endFound) {
        size_t want = cfg::kReadBlockBytes;
        size_t got = 0;
        r = file.read(block, want, got);
        if (!isOk(r)) return Result::ErrIO;
        if (got == 0) break;
        for (size_t i = 0; i < got && !endFound; ++i) {
            uint8_t b = block[i];
            uint64_t byteOff = pos + i;
            if (b == '\n') {
                if (containsCI(line, lineLen, "PROJECT GUTENBERG EBOOK") &&
                    containsCI(line, lineLen, "*** END OF T")) {
                    out.bodyEnd = lineStart;
                    endFound = true;
                    break;
                }
                lineLen = 0;
                lineStart = byteOff + 1;
            } else {
                if (lineLen < cfg::kScanLineMax) line[lineLen++] = static_cast<char>(b);
            }
        }
        pos += got;
    }
    if (!endFound && lineLen > 0) {
        if (containsCI(line, lineLen, "PROJECT GUTENBERG EBOOK") &&
            containsCI(line, lineLen, "*** END OF T")) {
            out.bodyEnd = lineStart;
        }
    }

    if (out.bodyEnd < out.bodyStart) out.bodyEnd = fileSize;
    return Result::Ok;
}

}  // namespace ereader
