// clock.hpp — Reloj monótono inyectable.
//
// El núcleo no depende de esp_timer ni de <chrono>. La plataforma instala aquí
// una función que devuelve microsegundos monótonos; si no se instala, devuelve
// 0 y las mediciones de tiempo quedan en 0 (sin afectar a la lógica).
#pragma once

#include <cstdint>

namespace ereader {

using NowMicrosFn = uint64_t (*)();

// Puntero global instalado por la plataforma (native: <chrono>; ESP32: esp_timer).
extern NowMicrosFn g_nowMicros;

inline uint64_t nowMicros() { return g_nowMicros ? g_nowMicros() : 0; }

}  // namespace ereader
