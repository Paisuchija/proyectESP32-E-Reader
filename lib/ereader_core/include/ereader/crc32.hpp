// crc32.hpp — CRC32 (polinomio IEEE 802.3, reflejado, igual que zlib).
//
// Se usa para el BookId, la integridad de índices y la recuperación atómica
// de progreso/marcadores. Implementación incremental para poder calcularlo
// sobre bloques sin cargar el archivo entero (regla §6).
#pragma once

#include <cstddef>
#include <cstdint>

namespace ereader {

class Crc32 {
public:
    Crc32() = default;

    // Reinicia el acumulador.
    void reset() { state_ = 0xFFFFFFFFu; }

    // Añade un bloque de datos al cálculo.
    void update(const void* data, size_t len);

    // Valor final del CRC (no modifica el estado interno salvo el xor final).
    uint32_t value() const { return state_ ^ 0xFFFFFFFFu; }

    // Atajo para calcular el CRC de un bloque de una sola vez.
    static uint32_t compute(const void* data, size_t len) {
        Crc32 c;
        c.update(data, len);
        return c.value();
    }

private:
    uint32_t state_ = 0xFFFFFFFFu;
};

}  // namespace ereader
