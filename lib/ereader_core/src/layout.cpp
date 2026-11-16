// layout.cpp — Implementación del motor de líneas y paginación.
#include "ereader/layout.hpp"

namespace ereader {

void LayoutEngine::init(ITokenSource* toks, const Geometry& geo,
                        const FontMetrics* fm, bool firstStartsParagraph) {
    toks_ = toks;
    geo_ = geo;
    fm_ = fm;
    spaceAdvance_ = fm ? fm->advance(0x20) : 0;
    pendingParaStart_ = firstStartsParagraph;
    pendingGap_ = false;
    wLen_ = 0;
    wWidth_ = 0;
    wValid_ = false;
    haveCarry_ = false;
    atEnd_ = false;
}

void LayoutEngine::readWord() {
    wLen_ = 0;
    wWidth_ = 0;
    bool first = true;
    Token t;
    for (;;) {
        Result r = toks_->peek(t);
        if (!isOk(r)) break;
        if (t.kind != TokKind::Glyph && t.kind != TokKind::Nbsp) break;
        toks_->next(t);
        if (first) { wStartOff_ = t.offset; first = false; }
        if (wLen_ >= static_cast<int>(cfg::kMaxGlyphsPerLine)) break;
        wCp_[wLen_] = t.cp;
        int32_t a = fm_->advance(t.cp);
        wAdv_[wLen_] = a;
        wOff_[wLen_] = t.offset;
        wWidth_ += a;
        ++wLen_;
        if (wLen_ == static_cast<int>(cfg::kMaxGlyphsPerLine)) break;  // palabra gigante
    }
    wValid_ = (wLen_ > 0);
}

Result LayoutEngine::nextLine(Line& out) {
    out.count = 0;
    out.width = 0;
    out.startOffset = 0;
    out.startsParagraph = pendingParaStart_;
    out.paragraphGapBefore = pendingGap_;
    int32_t indent = pendingParaStart_ ? geo_.indent : 0;
    pendingParaStart_ = false;
    pendingGap_ = false;

    const int32_t limit = geo_.textWidth();
    int32_t x = indent;
    bool pendingSpace = false;
    bool lineHasOffset = false;

    for (;;) {
        if (!wValid_) {
            Token t;
            Result r = toks_->peek(t);
            if (!isOk(r)) return r;
            if (t.kind == TokKind::End) {
                if (out.count == 0 && !lineHasOffset) return Result::End;
                break;
            }
            if (t.kind == TokKind::Space) {
                toks_->next(t);
                if (out.count > 0) pendingSpace = true;  // colapsa; irrelevante en vacío
                continue;
            }
            if (t.kind == TokKind::LineBreak) {
                toks_->next(t);
                if (!lineHasOffset) {  // línea en blanco: conserva un offset válido
                    out.startOffset = t.offset;
                    lineHasOffset = true;
                }
                break;  // salto forzado (puede ser línea vacía)
            }
            if (t.kind == TokKind::ParaBreak) {
                toks_->next(t);
                if (out.count == 0) {
                    // Saltos de párrafo consecutivos: colapsar, sigue siendo
                    // inicio de párrafo. Reaplicar la sangría a la línea actual.
                    out.startsParagraph = true;
                    out.paragraphGapBefore = true;
                    indent = geo_.indent;
                    x = indent;
                    continue;
                }
                pendingParaStart_ = true;
                pendingGap_ = true;
                break;
            }
            // Glyph o Nbsp.
            readWord();
        }

        // Colocar la palabra pendiente.
        int32_t sepW = (out.count > 0 && pendingSpace) ? spaceAdvance_ : 0;
        if (out.count > 0 && x + sepW + wWidth_ > limit) {
            break;  // la palabra entera no cabe -> se arrastra; el salto reemplaza el espacio
        }
        if (sepW > 0) {
            out.glyphs[out.count].cp = 0x20;
            out.glyphs[out.count].x = x;
            ++out.count;
            x += sepW;
            pendingSpace = false;
        }
        if (!lineHasOffset) {
            out.startOffset = wStartOff_;
            lineHasOffset = true;
        }
        int i = 0;
        bool forced = false;
        for (; i < wLen_; ++i) {
            if (out.count > 0 && x + wAdv_[i] > limit) { forced = true; break; }
            if (out.count >= static_cast<int>(cfg::kMaxGlyphsPerLine)) { forced = true; break; }
            out.glyphs[out.count].cp = wCp_[i];
            out.glyphs[out.count].x = x;
            ++out.count;
            x += wAdv_[i];
        }
        if (forced) {
            int rem = wLen_ - i;
            for (int k = 0; k < rem; ++k) {
                wCp_[k] = wCp_[i + k];
                wAdv_[k] = wAdv_[i + k];
                wOff_[k] = wOff_[i + k];
            }
            wStartOff_ = (rem > 0) ? wOff_[0] : wStartOff_;
            wLen_ = rem;
            wWidth_ = 0;
            for (int k = 0; k < rem; ++k) wWidth_ += wAdv_[k];
            wValid_ = (rem > 0);
            break;
        }
        wValid_ = false;
    }

    out.width = x;
    return Result::Ok;
}

Result LayoutEngine::nextPage(Page& out) {
    out.clear();
    if (atEnd_ && !haveCarry_) return Result::End;

    const int32_t maxY = geo_.height - geo_.marginBottom;
    const int32_t lh = fm_->lineHeight();
    int32_t y = geo_.marginTop;

    for (;;) {
        if (!haveCarry_) {
            Result r = nextLine(carryLine_);
            if (r == Result::End) { atEnd_ = true; break; }
            if (!isOk(r)) return r;
            haveCarry_ = true;
        }

        // Saltar líneas vacías que caerían al inicio de la página.
        if (out.lineCount == 0 && carryLine_.count == 0) {
            haveCarry_ = false;
            continue;
        }

        int32_t gap = 0;
        if (out.lineCount > 0 && carryLine_.paragraphGapBefore) gap = geo_.paragraphGap;
        int32_t top = y + gap;
        if (out.lineCount > 0 && top + lh > maxY) {
            break;  // no cabe: se queda para la próxima página (carry retenido)
        }
        carryLine_.y = top;
        out.lines[out.lineCount] = carryLine_;
        if (out.lineCount == 0) {
            out.startOffset = carryLine_.startOffset;
            out.startsParagraph = carryLine_.startsParagraph;
        }
        ++out.lineCount;
        y = top + lh + geo_.lineGap;
        haveCarry_ = false;
        if (out.lineCount >= static_cast<int>(cfg::kMaxLinesPerPage)) break;
    }

    if (out.lineCount == 0) return Result::End;
    return Result::Ok;
}

}  // namespace ereader
