// reader_session.hpp — Sesión de lectura de un libro (§7.2 ReaderSession).
//
// Orquesta el pipeline (Utf8Source -> Reflow -> LayoutEngine), el índice de
// páginas, el progreso y la navegación. Mantiene SIEMPRE una sola página
// maquetada en memoria (regla §6). La posición se guarda como offset en bytes.
#pragma once

#include <cstdint>

#include "ereader/book_id.hpp"
#include "ereader/font_metrics.hpp"
#include "ereader/gutenberg_info.hpp"
#include "ereader/layout.hpp"
#include "ereader/layout_config.hpp"
#include "ereader/page.hpp"
#include "ereader/page_index.hpp"
#include "ereader/progress.hpp"
#include "ereader/text_stream.hpp"
#include "ereader/utf8_source.hpp"

namespace ereader {

// Token de cancelación cooperativo para el indexado en segundo plano (§7.3).
struct CancelToken {
    volatile bool cancelled = false;
    void cancel() { cancelled = true; }
    bool isCancelled() const { return cancelled; }
};

class ReaderSession {
public:
    ReaderSession() = default;
    ~ReaderSession() { close(); }

    // Abre `bookPath`. `fm` debe sobrevivir a la sesión. Posiciona en el
    // progreso guardado (o al inicio). No construye el índice (ver buildIndex).
    // `sysRoot` es el directorio de cachés (índices/progreso); en la SD del
    // dispositivo es "/sys" (§4: nunca en flash), configurable para el PC.
    Result open(IFileSystem& fs, const char* bookPath, const LayoutConfig& cfg,
                const FontMetrics& fm, const char* sysRoot = "/sys");
    void close();

    // Navegación. Devuelven Result::End si no hay más (se quedan en el borde).
    Result nextPage();
    Result prevPage();
    // Va al porcentaje [0..1000] (por mil) del cuerpo.
    Result gotoPermille(uint32_t permille);
    // Va a un offset de byte arbitrario (lo ajusta al inicio de página).
    Result gotoOffset(uint64_t byteOffset);

    // La página actual ya maquetada.
    const Page& currentPage() const { return page_; }
    uint32_t lastLayoutMicros() const { return layoutMicros_; }

    // Porcentaje leído ×10 (0..1000). Por bytes si el índice no está completo;
    // por página/total si lo está.
    uint32_t percentX10() const;
    // Número de página 1-based si se conoce (0 si no). Total si índice completo.
    uint32_t currentPageNumber() const { return pageNumberKnown_ ? pageNumber_ : 0; }
    uint32_t pageTotal() const { return index_.complete() ? index_.pageCount() : 0; }
    bool indexing() const { return !index_.complete(); }

    const GutenbergInfo& info() const { return gi_; }
    const BookId& bookId() const { return id_; }
    uint64_t currentOffset() const { return currentOffset_; }

    // Construye el índice COMPLETO en primer plano (M1/M2). Cede vía `cancel`.
    Result buildIndexForeground(CancelToken* cancel = nullptr);
    // Da un paso de indexado (para la tarea en segundo plano, M3). Devuelve End
    // cuando termina (y finaliza el índice). `pagesPerStep` páginas por llamada.
    Result indexStep(uint32_t pagesPerStep, CancelToken* cancel = nullptr);

    // Guarda el progreso de forma atómica en la posición actual.
    Result saveProgress(uint64_t timestampMs = 0);

    IFileSystem* fs() const { return fs_; }
    const char* indexPath() const { return idxPath_; }

private:
    void renderCurrent();
    void pushHistory(uint64_t off, bool para);
    bool popHistory(uint64_t& off, bool& para);
    // Pagina desde bodyStart hasta encontrar la página que contiene `target`;
    // deja current en el inicio de esa página (fallback sin índice).
    Result scanToOffset(uint64_t target);
    // Offset del inicio del índice/progreso del libro. Crea /sys/<id>/ .
    void buildPaths();

    IFileSystem* fs_ = nullptr;
    IFile* bookFile_ = nullptr;
    const FontMetrics* fm_ = nullptr;
    LayoutConfig cfg_{};
    uint32_t layoutHash_ = 0;

    BookId id_{};
    GutenbergInfo gi_{};

    Utf8Source src_;
    ReflowFilter rf_;
    LayoutEngine le_;
    PageIndex index_;
    Page page_{};

    uint64_t currentOffset_ = 0;
    bool currentStartsParagraph_ = true;
    uint64_t nextOffset_ = 0;
    bool nextStartsParagraph_ = false;
    bool hasNext_ = false;
    uint32_t layoutMicros_ = 0;

    uint32_t pageNumber_ = 1;       // 1-based, cuando se conoce.
    bool pageNumberKnown_ = true;

    // Indexado incremental (usa su PROPIO handle de archivo para no competir
    // con el cursor de la sesión de lectura).
    bool indexBuilding_ = false;
    IFile* idxFile_ = nullptr;
    LayoutEngine idxEngine_;
    Utf8Source idxSrc_;
    ReflowFilter idxRf_;
    Page idxPage_{};

    // Historial de páginas visitadas (para "anterior" barato y exacto).
    static constexpr int kHistory = 128;
    uint64_t histOff_[kHistory];
    bool histPara_[kHistory];
    int histCount_ = 0;
    int histHead_ = 0;

    char bookPath_[cfg::kMaxPathLen];
    char sysRoot_[cfg::kMaxPathLen];
    char sysPath_[cfg::kMaxPathLen];
    char idxPath_[cfg::kMaxPathLen];
    char progPath_[cfg::kMaxPathLen];
};

}  // namespace ereader
