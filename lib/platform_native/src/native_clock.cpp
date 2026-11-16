// native_clock.cpp — Reloj monótono con std::chrono.
#include "platform_native/native_clock.hpp"

#include <chrono>

#include "ereader/clock.hpp"

namespace ereader {
namespace native {

static uint64_t nowMicrosImpl() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

void installNativeClock() { ereader::g_nowMicros = &nowMicrosImpl; }

}  // namespace native
}  // namespace ereader
