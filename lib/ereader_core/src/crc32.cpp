// crc32.cpp — Implementación del CRC32 sin tabla precomputada grande.
//
// Se usa el algoritmo "slicing" bit a bit con el polinomio reflejado
// 0xEDB88320. Es suficientemente rápido para bloques de 4 KB y evita una
// tabla de 1 KB en flash; si el rendimiento del indexado lo exigiera, se
// puede sustituir por una tabla de 256 entradas sin cambiar la API.
#include "ereader/crc32.hpp"

namespace ereader {

void Crc32::update(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint32_t crc = state_;
    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (int bit = 0; bit < 8; ++bit) {
            // Si el bit bajo está activo, desplaza y aplica el polinomio.
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    state_ = crc;
}

}  // namespace ereader
