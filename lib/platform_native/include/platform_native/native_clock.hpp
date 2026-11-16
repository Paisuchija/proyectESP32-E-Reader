// native_clock.hpp — Instala un reloj monótono para el entorno PC.
#pragma once

namespace ereader {
namespace native {

// Instala ereader::g_nowMicros usando std::chrono (steady_clock).
void installNativeClock();

}  // namespace native
}  // namespace ereader
