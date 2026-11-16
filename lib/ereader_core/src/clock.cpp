// clock.cpp — Definición del puntero de reloj global.
#include "ereader/clock.hpp"

namespace ereader {
NowMicrosFn g_nowMicros = nullptr;
}  // namespace ereader
