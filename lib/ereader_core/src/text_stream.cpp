// text_stream.cpp — Implementación del filtro de reflow.
#include "ereader/text_stream.hpp"

namespace ereader {

namespace {
constexpr uint32_t kLF = 0x0A;
constexpr uint32_t kNbsp = 0x00A0;
bool isCollapsibleSpace(uint32_t cp) { return cp == ' ' || cp == '\t'; }
}  // namespace

void ReflowFilter::init(ICodepointSource* src, bool reflow) {
    src_ = src;
    reflow_ = reflow;
    emittedContent_ = false;
    hasPeek_ = false;
}

Result ReflowFilter::next(Token& out) {
    if (hasPeek_) {
        out = peek_;
        hasPeek_ = false;
        return Result::Ok;
    }
    return produce(out);
}

Result ReflowFilter::peek(Token& out) {
    if (!hasPeek_) {
        Result r = produce(peek_);
        if (!isOk(r)) return r;
        hasPeek_ = true;
    }
    out = peek_;
    return Result::Ok;
}

Result ReflowFilter::produce(Token& out) {
    Cp c;
    if (reflow_) {
        // 1) Consumir un posible bloque de espacios/saltos y clasificarlo.
        int newlines = 0;
        bool anyWs = false;
        uint64_t wsOff = 0;
        for (;;) {
            Result r = src_->peek(c);
            if (r == Result::End) break;
            if (!isOk(r)) return r;
            if (c.cp == kLF) {
                if (!anyWs) wsOff = c.offset;
                anyWs = true;
                ++newlines;
                src_->next(c);
            } else if (isCollapsibleSpace(c.cp)) {
                if (!anyWs) wsOff = c.offset;
                anyWs = true;
                src_->next(c);
            } else {
                break;  // Contenido (glifo o NBSP).
            }
        }
        if (anyWs && emittedContent_) {
            // Un bloque de espacios entre dos contenidos -> un solo token.
            if (newlines >= 2) {
                out = Token{TokKind::ParaBreak, 0, wsOff};
            } else {
                out = Token{TokKind::Space, ' ', wsOff};
            }
            return Result::Ok;
        }
        // Si anyWs && !emittedContent_: eran espacios iniciales -> descartados.
    } else {
        // Reflow OFF: recortar solo los espacios/saltos iniciales del cuerpo.
        if (!emittedContent_) {
            for (;;) {
                Result r = src_->peek(c);
                if (r == Result::End) break;
                if (!isOk(r)) return r;
                if (c.cp == kLF || isCollapsibleSpace(c.cp)) {
                    src_->next(c);
                } else {
                    break;
                }
            }
        }
    }

    // 2) Contenido o fin de stream.
    Result r = src_->peek(c);
    if (r == Result::End) {
        out = Token{TokKind::End, 0, 0};
        return Result::Ok;
    }
    if (!isOk(r)) return r;
    src_->next(c);

    if (!reflow_) {
        if (c.cp == kLF) {
            out = Token{TokKind::LineBreak, 0, c.offset};
            return Result::Ok;
        }
        if (isCollapsibleSpace(c.cp)) {
            out = Token{TokKind::Space, ' ', c.offset};
            return Result::Ok;
        }
    }

    emittedContent_ = true;
    if (c.cp == kNbsp) {
        out = Token{TokKind::Nbsp, kNbsp, c.offset};
    } else {
        out = Token{TokKind::Glyph, c.cp, c.offset};
    }
    return Result::Ok;
}

}  // namespace ereader
