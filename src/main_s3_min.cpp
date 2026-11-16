// main_s3_min.cpp — Comprobación de compilación del núcleo para el ESP32-S3.
//
// No implementa funcionalidad: solo instancia el pipeline del núcleo para
// garantizar DESDE M1 que lib/ereader_core compila en el toolchain del S3
// (qio_opi + PSRAM) sin depender de nada específico de la DevKit (§7.1).
#include <Arduino.h>

#include "ereader/crc32.hpp"
#include "ereader/layout.hpp"
#include "ereader/layout_config.hpp"
#include "ereader/reader_session.hpp"
#include "ereader/utf8_decoder.hpp"

using namespace ereader;

void setup() {
    Serial.begin(115200);

    // Referencias para forzar la instanciación/enlace del núcleo.
    Utf8Decoder dec;
    uint32_t out[4];
    uint64_t off[4];
    size_t n = 0;
    const uint8_t sample[] = {'O', 'K'};
    dec.decode(sample, sizeof(sample), 0, out, off, 4, n);

    LayoutConfig cfg;
    volatile uint32_t h = cfg.hash();
    (void)h;
    (void)n;

    Serial.printf("ereader_core OK (crc=%08x)\n", Crc32::compute(sample, sizeof(sample)));
}

void loop() { delay(1000); }
