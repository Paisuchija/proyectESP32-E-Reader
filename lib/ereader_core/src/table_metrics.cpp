// table_metrics.cpp — Implementación de TableMetrics (lectura de ERFM).
#include "ereader/table_metrics.hpp"

#include <cstring>

#include "ereader/byteio.hpp"

namespace ereader {

Result TableMetrics::init(const uint8_t* data, size_t len) {
    rangeCount_ = 0;
    data_ = nullptr;
    if (!data || len < 24) return Result::ErrInvalidArg;
    if (std::memcmp(data, "ERFM", 4) != 0) return Result::ErrCorrupt;
    if (bio::getU16(data + 4) != 1) return Result::ErrCorrupt;

    sizePx_ = bio::getU16(data + 6);
    lineHeight_ = bio::getU16(data + 8);
    ascent_ = bio::getU16(data + 10);
    descent_ = bio::getU16(data + 12);
    replAdvance_ = bio::getU16(data + 14);
    int nranges = bio::getU16(data + 16);
    if (nranges > kMaxRanges) return Result::ErrCorrupt;

    size_t pos = 24;
    for (int i = 0; i < nranges; ++i) {
        if (pos + 6 > len) return Result::ErrCorrupt;
        uint32_t first = bio::getU32(data + pos);
        uint16_t count = bio::getU16(data + pos + 4);
        pos += 6;
        if (pos + static_cast<size_t>(count) * 2 > len) return Result::ErrCorrupt;
        ranges_[i].first = first;
        ranges_[i].count = count;
        ranges_[i].adv = data + pos;
        pos += static_cast<size_t>(count) * 2;
    }
    rangeCount_ = nranges;
    data_ = data;
    id_ = 0x7AB00000u ^ static_cast<uint32_t>(sizePx_) ^
          (static_cast<uint32_t>(lineHeight_) << 8) ^
          (static_cast<uint32_t>(replAdvance_) << 16);
    return Result::Ok;
}

int32_t TableMetrics::advance(uint32_t cp) const {
    for (int i = 0; i < rangeCount_; ++i) {
        const Range& r = ranges_[i];
        if (cp >= r.first && cp < r.first + r.count) {
            return bio::getU16(r.adv + (cp - r.first) * 2);
        }
    }
    return replAdvance_;
}

}  // namespace ereader
