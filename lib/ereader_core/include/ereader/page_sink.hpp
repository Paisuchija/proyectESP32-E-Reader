// page_sink.hpp — Interfaz de renderizado de páginas (§7.2 PageSink).
//
// Recibe cada página YA maquetada (líneas con la posición x/y de cada glifo).
// Hoy la implementa un sink de texto (consola serie / stdout); mañana un
// renderer sobre framebuffer RGB565 con glifos suavizados. El núcleo no sabe
// a dónde va la salida.
#pragma once

#include <cstdint>

#include "ereader/page.hpp"

namespace ereader {

// Cabecera de la página para mostrar al usuario.
struct PageHeader {
    const char* title = "";
    const char* author = "";
    uint32_t pageNumber = 0;     // 1-based (0 = desconocido).
    uint32_t pageTotal = 0;      // 0 si el índice aún no está completo.
    uint32_t percentX10 = 0;     // Porcentaje leído ×10 (p. ej. 123 = 12,3 %).
    bool indexing = false;       // true mientras se construye el índice (~%).
    uint32_t layoutMicros = 0;   // Tiempo de cálculo de layout (µs).
};

class PageSink {
public:
    virtual ~PageSink() = default;
    // Dibuja la página con su cabecera. El sink puede medir su propio tiempo
    // de "impresión" por separado del layout.
    virtual void render(const Page& page, const PageHeader& header) = 0;
};

// Sink de texto reutilizable: formatea la página en UTF-8 y la entrega a un
// callback de escritura (stdout en el PC, Serial en el ESP32).
class CallbackPageSink : public PageSink {
public:
    using WriteFn = void (*)(const char* data, size_t len, void* user);

    CallbackPageSink(WriteFn fn, void* user) : fn_(fn), user_(user) {}

    void render(const Page& page, const PageHeader& header) override;

private:
    void writeStr(const char* s);
    WriteFn fn_;
    void* user_;
};

}  // namespace ereader
