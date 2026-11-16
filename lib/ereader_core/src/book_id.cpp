// book_id.cpp — Implementación de BookId.
#include "ereader/book_id.hpp"

#include <cstdio>

#include "ereader/config.hpp"
#include "ereader/crc32.hpp"

namespace ereader {

Result computeBookId(IFile& file, BookId& out) {
    out.size = file.size();
    Result r = file.seek(0, Seek::Set);
    if (!isOk(r)) return r;

    Crc32 crc;
    uint8_t buf[cfg::kReadBlockBytes];
    size_t remaining = cfg::kBookIdPrefixBytes;
    while (remaining > 0) {
        size_t want = remaining < sizeof(buf) ? remaining : sizeof(buf);
        size_t got = 0;
        r = file.read(buf, want, got);
        if (!isOk(r)) return r;
        if (got == 0) break;  // Archivo más corto que el prefijo.
        crc.update(buf, got);
        remaining -= got;
    }
    out.crc = crc.value();
    return Result::Ok;
}

char* BookId::toString(char* buf, size_t cap) const {
    if (cap >= 24) {
        std::snprintf(buf, cap, "%08x%08x",
                      crc, static_cast<uint32_t>(size & 0xFFFFFFFFu));
    } else if (cap > 0) {
        buf[0] = '\0';
    }
    return buf;
}

}  // namespace ereader
