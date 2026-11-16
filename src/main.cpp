// main.cpp — Aplicación de consola serie en la ESP32 DevKit V1.
//
// M1: arranque mínimo que verifica que el núcleo enlaza bajo Arduino-ESP32.
// M2 añadirá el montaje de la SD, el SdFileSystem y la consola de comandos
// (ls/open/n/p/g/info/layout/reflow). Se mantiene compilable en cada hito.
#include <Arduino.h>
#include <esp_timer.h>

#include "ereader/crc32.hpp"
#include "ereader/clock.hpp"

using namespace ereader;

// Reloj monótono del ESP32 para las mediciones del núcleo (esp_timer).
static uint64_t espNowMicros() { return static_cast<uint64_t>(esp_timer_get_time()); }

void setup() {
    Serial.begin(115200);
    delay(200);
    ereader::g_nowMicros = &espNowMicros;

    Serial.println();
    Serial.println("E-reader ESP32 — Fase 1 (M1: nucleo enlazado)");
    const uint8_t probe[] = {'e', 'r', 'e', 'a', 'd', 'e', 'r'};
    Serial.printf("nucleo OK, crc=%08x, heap libre=%u B\n",
                  Crc32::compute(probe, sizeof(probe)),
                  static_cast<unsigned>(ESP.getFreeHeap()));
    Serial.println("La consola de comandos llega en M2.");
}

void loop() { delay(1000); }
