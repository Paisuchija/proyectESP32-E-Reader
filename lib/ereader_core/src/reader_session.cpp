// reader_session.cpp — Implementación de la sesión de lectura.
#include "ereader/reader_session.hpp"

#include <cstdio>
#include <cstring>

#include "ereader/clock.hpp"

namespace ereader {

void ReaderSession::buildPaths() {
    char idhex[24];
    id_.toString(idhex, sizeof(idhex));
    fs_->mkdir(sysRoot_);
    std::snprintf(sysPath_, sizeof(sysPath_), "%s/%s", sysRoot_, idhex);
    fs_->mkdir(sysPath_);
    std::snprintf(idxPath_, sizeof(idxPath_), "%s/i%08x.idx", sysPath_, layoutHash_);
    std::snprintf(progPath_, sizeof(progPath_), "%s/progress.prg", sysPath_);
}

Result ReaderSession::open(IFileSystem& fs, const char* bookPath,
                           const LayoutConfig& cfg, const FontMetrics& fm,
                           const char* sysRoot) {
    close();
    fs_ = &fs;
    fm_ = &fm;
    cfg_ = cfg;
    cfg_.fontId = fm.id();  // la identidad real de la fuente manda
    layoutHash_ = cfg_.hash();
    std::snprintf(bookPath_, sizeof(bookPath_), "%s", bookPath);
    std::snprintf(sysRoot_, sizeof(sysRoot_), "%s", sysRoot);

    Result r = fs.open(bookPath, OpenMode::Read, bookFile_);
    if (!isOk(r)) return r;
    r = computeBookId(*bookFile_, id_);
    if (!isOk(r)) return r;
    r = detectGutenberg(*bookFile_, gi_);
    if (!isOk(r)) return r;
    r = src_.init(bookFile_, gi_.bodyStart, gi_.bodyEnd);
    if (!isOk(r)) return r;

    buildPaths();
    index_.openRead(fs, idxPath_, id_, layoutHash_);

    histCount_ = 0;
    histHead_ = 0;

    // Cargar progreso.
    Progress p;
    if (isOk(ProgressStore::load(fs, progPath_, id_, p)) &&
        p.byteOffset > gi_.bodyStart && p.byteOffset < gi_.bodyEnd) {
        return gotoOffset(p.byteOffset);
    }
    // Empezar al inicio.
    currentOffset_ = gi_.bodyStart;
    currentStartsParagraph_ = true;
    pageNumber_ = 1;
    pageNumberKnown_ = true;
    renderCurrent();
    return Result::Ok;
}

void ReaderSession::close() {
    index_.close();
    if (idxFile_) { idxFile_->close(); delete idxFile_; idxFile_ = nullptr; }
    if (bookFile_) { bookFile_->close(); delete bookFile_; bookFile_ = nullptr; }
    fs_ = nullptr;
    indexBuilding_ = false;
    histCount_ = 0;
    histHead_ = 0;
}

void ReaderSession::renderCurrent() {
    src_.seekTo(currentOffset_);
    rf_.init(&src_, cfg_.reflow);
    le_.init(&rf_, cfg_.geo, fm_, currentStartsParagraph_);
    uint64_t t0 = nowMicros();
    le_.nextPage(page_);
    layoutMicros_ = static_cast<uint32_t>(nowMicros() - t0);
    hasNext_ = le_.hasNextPage();
    nextOffset_ = le_.nextPageOffset();
    nextStartsParagraph_ = le_.nextPageStartsParagraph();
}

void ReaderSession::pushHistory(uint64_t off, bool para) {
    histOff_[histHead_] = off;
    histPara_[histHead_] = para;
    histHead_ = (histHead_ + 1) % kHistory;
    if (histCount_ < kHistory) ++histCount_;
}

bool ReaderSession::popHistory(uint64_t& off, bool& para) {
    if (histCount_ == 0) return false;
    histHead_ = (histHead_ - 1 + kHistory) % kHistory;
    off = histOff_[histHead_];
    para = histPara_[histHead_];
    --histCount_;
    return true;
}

Result ReaderSession::nextPage() {
    if (!hasNext_) return Result::End;
    pushHistory(currentOffset_, currentStartsParagraph_);
    currentOffset_ = nextOffset_;
    currentStartsParagraph_ = nextStartsParagraph_;
    if (pageNumberKnown_) ++pageNumber_;
    renderCurrent();
    return Result::Ok;
}

Result ReaderSession::prevPage() {
    uint64_t off;
    bool para;
    if (popHistory(off, para)) {
        currentOffset_ = off;
        currentStartsParagraph_ = para;
        if (pageNumberKnown_ && pageNumber_ > 1) --pageNumber_;
        renderCurrent();
        return Result::Ok;
    }
    if (currentOffset_ <= gi_.bodyStart) return Result::End;

    // Sin historial: localizar la página anterior.
    if (index_.complete()) {
        uint32_t idx;
        if (isOk(index_.findPage(currentOffset_, idx)) && idx > 0) {
            uint64_t po;
            uint16_t fl;
            if (isOk(index_.getEntry(idx - 1, po, fl))) {
                currentOffset_ = po;
                currentStartsParagraph_ = (fl & kPageFlagParaStart) != 0;
                pageNumber_ = idx;  // idx-1 en 0-based -> idx en 1-based
                pageNumberKnown_ = true;
                renderCurrent();
                return Result::Ok;
            }
        }
        return Result::End;
    }
    // Fallback sin índice: paginar desde el inicio buscando la página previa.
    uint64_t prevOff = gi_.bodyStart;
    bool prevPara = true;
    uint32_t num = 1;
    Utf8Source s;
    s.init(bookFile_, gi_.bodyStart, gi_.bodyEnd);
    ReflowFilter rf;
    rf.init(&s, cfg_.reflow);
    LayoutEngine le;
    le.init(&rf, cfg_.geo, fm_, true);
    for (;;) {
        Page tmp;
        Result r = le.nextPage(tmp);
        if (r == Result::End) break;
        if (tmp.startOffset >= currentOffset_) break;
        prevOff = tmp.startOffset;
        prevPara = tmp.startsParagraph;
        if (le.hasNextPage() && le.nextPageOffset() >= currentOffset_) {
            num = 0;  // no sabemos el número exacto de forma barata; dejamos desconocido
            break;
        }
    }
    currentOffset_ = prevOff;
    currentStartsParagraph_ = prevPara;
    pageNumberKnown_ = false;
    (void)num;
    renderCurrent();
    return Result::Ok;
}

Result ReaderSession::scanToOffset(uint64_t target) {
    // Pagina desde bodyStart hasta la página que contiene `target`.
    Utf8Source s;
    s.init(bookFile_, gi_.bodyStart, gi_.bodyEnd);
    ReflowFilter rf;
    rf.init(&s, cfg_.reflow);
    LayoutEngine le;
    le.init(&rf, cfg_.geo, fm_, true);
    uint64_t foundOff = gi_.bodyStart;
    bool foundPara = true;
    uint32_t num = 0;
    for (;;) {
        Page tmp;
        Result r = le.nextPage(tmp);
        if (r == Result::End) break;
        ++num;
        foundOff = tmp.startOffset;
        foundPara = tmp.startsParagraph;
        uint64_t end = le.hasNextPage() ? le.nextPageOffset() : gi_.bodyEnd;
        if (target < end) break;  // esta página contiene target
    }
    currentOffset_ = foundOff;
    currentStartsParagraph_ = foundPara;
    pageNumber_ = num;
    pageNumberKnown_ = (num > 0);
    renderCurrent();
    return Result::Ok;
}

Result ReaderSession::gotoOffset(uint64_t byteOffset) {
    if (byteOffset < gi_.bodyStart) byteOffset = gi_.bodyStart;
    if (byteOffset > gi_.bodyEnd) byteOffset = gi_.bodyEnd;
    histCount_ = 0;
    histHead_ = 0;

    if (byteOffset <= gi_.bodyStart) {
        currentOffset_ = gi_.bodyStart;
        currentStartsParagraph_ = true;
        pageNumber_ = 1;
        pageNumberKnown_ = true;
        renderCurrent();
        return Result::Ok;
    }

    // Vía índice si cubre el objetivo.
    if (index_.pageCount() > 0) {
        uint64_t lastOff;
        uint16_t fl;
        index_.getEntry(index_.pageCount() - 1, lastOff, fl);
        if (index_.complete() || byteOffset <= lastOff) {
            uint32_t idx;
            if (isOk(index_.findPage(byteOffset, idx))) {
                uint64_t po;
                index_.getEntry(idx, po, fl);
                currentOffset_ = po;
                currentStartsParagraph_ = (fl & kPageFlagParaStart) != 0;
                pageNumber_ = idx + 1;
                pageNumberKnown_ = true;
                renderCurrent();
                return Result::Ok;
            }
        }
    }
    return scanToOffset(byteOffset);
}

Result ReaderSession::gotoPermille(uint32_t permille) {
    if (permille > 1000) permille = 1000;
    uint64_t span = gi_.bodyEnd - gi_.bodyStart;
    uint64_t target = gi_.bodyStart + span * permille / 1000;
    return gotoOffset(target);
}

uint32_t ReaderSession::percentX10() const {
    if (index_.complete() && index_.pageCount() > 0 && pageNumberKnown_) {
        return static_cast<uint32_t>(static_cast<uint64_t>(pageNumber_) * 1000 /
                                     index_.pageCount());
    }
    uint64_t span = gi_.bodyEnd - gi_.bodyStart;
    if (span == 0) return 0;
    return static_cast<uint32_t>((currentOffset_ - gi_.bodyStart) * 1000 / span);
}

Result ReaderSession::buildIndexForeground(CancelToken* cancel) {
    if (index_.complete()) return Result::Ok;
    uint32_t resumePage;
    uint64_t resumeOff;
    bool resumePara;
    Result r = index_.createOrResume(*fs_, idxPath_, id_, layoutHash_, gi_.bodyStart,
                                     gi_.bodyEnd, resumePage, resumeOff, resumePara);
    if (!isOk(r)) return r;

    if (idxFile_) { idxFile_->close(); delete idxFile_; idxFile_ = nullptr; }
    r = fs_->open(bookPath_, OpenMode::Read, idxFile_);
    if (!isOk(r)) return r;
    idxSrc_.init(idxFile_, gi_.bodyStart, gi_.bodyEnd);
    idxSrc_.seekTo(resumeOff);
    idxRf_.init(&idxSrc_, cfg_.reflow);
    idxEngine_.init(&idxRf_, cfg_.geo, fm_, resumePara);

    for (;;) {
        r = idxEngine_.nextPage(idxPage_);
        if (r == Result::End) break;
        if (!isOk(r)) { index_.flushHeader(); return r; }
        uint16_t flags = idxPage_.startsParagraph ? kPageFlagParaStart : 0;
        r = index_.appendPage(idxPage_.startOffset, flags);
        if (!isOk(r)) return r;
        if (cancel && cancel->isCancelled()) {
            index_.flushHeader();
            idxFile_->close();
            delete idxFile_;
            idxFile_ = nullptr;
            return Result::ErrCancelled;
        }
    }
    r = index_.finalize();
    idxFile_->close();
    delete idxFile_;
    idxFile_ = nullptr;

    // Reabrir el índice para lectura (findPage, etc.).
    index_.close();
    index_.openRead(*fs_, idxPath_, id_, layoutHash_);
    return r;
}

Result ReaderSession::indexStep(uint32_t pagesPerStep, CancelToken* cancel) {
    if (index_.complete()) return Result::End;
    if (!indexBuilding_) {
        uint32_t resumePage;
        uint64_t resumeOff;
        bool resumePara;
        Result r = index_.createOrResume(*fs_, idxPath_, id_, layoutHash_, gi_.bodyStart,
                                         gi_.bodyEnd, resumePage, resumeOff, resumePara);
        if (!isOk(r)) return r;
        if (idxFile_) { idxFile_->close(); delete idxFile_; idxFile_ = nullptr; }
        r = fs_->open(bookPath_, OpenMode::Read, idxFile_);
        if (!isOk(r)) return r;
        idxSrc_.init(idxFile_, gi_.bodyStart, gi_.bodyEnd);
        idxSrc_.seekTo(resumeOff);
        idxRf_.init(&idxSrc_, cfg_.reflow);
        idxEngine_.init(&idxRf_, cfg_.geo, fm_, resumePara);
        indexBuilding_ = true;
    }
    for (uint32_t i = 0; i < pagesPerStep; ++i) {
        if (cancel && cancel->isCancelled()) {
            index_.flushHeader();
            return Result::ErrCancelled;
        }
        Result r = idxEngine_.nextPage(idxPage_);
        if (r == Result::End) {
            index_.finalize();
            idxFile_->close();
            delete idxFile_;
            idxFile_ = nullptr;
            indexBuilding_ = false;
            index_.close();
            index_.openRead(*fs_, idxPath_, id_, layoutHash_);
            return Result::End;
        }
        if (!isOk(r)) return r;
        uint16_t flags = idxPage_.startsParagraph ? kPageFlagParaStart : 0;
        index_.appendPage(idxPage_.startOffset, flags);
    }
    return Result::ErrAgain;  // quedan más páginas
}

Result ReaderSession::saveProgress(uint64_t timestampMs) {
    Progress p;
    p.byteOffset = currentOffset_;
    p.layoutHash = layoutHash_;
    p.timestampMs = timestampMs;
    return ProgressStore::save(*fs_, progPath_, id_, p);
}

}  // namespace ereader
