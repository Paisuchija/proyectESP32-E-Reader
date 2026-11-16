// utf8_decoder.hpp — Decodificador UTF-8 con estado entre bloques.
//
// Requisitos (§6, §10):
//  - Conserva estado: un carácter partido en el borde de un bloque NO se
//    pierde ni se corrompe.
//  - Salta el BOM inicial (U+FEFF).
//  - Secuencias inválidas u "overlong" -> U+FFFD, sin perder el byte que
//    interrumpe una secuencia (se reprocesa como nuevo inicio).
//  - Normaliza CRLF y CR sueltos a LF.
//  - Para cada codepoint emitido puede devolver el offset en bytes del PRIMER
//    byte de su secuencia en el archivo (necesario para identificar páginas).
//
// NO decodifica a std::string ni asigna memoria: escribe codepoints (uint32)
// en un buffer que aporta el llamante.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ereader/result.hpp"

namespace ereader {

class Utf8Decoder {
public:
    Utf8Decoder() { reset(true); }

    // Reinicia todo el estado. `atStart` indica si estamos al principio real
    // del archivo (offset 0): solo entonces se descarta un BOM inicial. Al
    // reiniciar en mitad del texto (p. ej. al repaginar desde el offset de una
    // página) debe ser false para no tragarse un carácter real.
    void reset(bool atStart = true);

    // Decodifica `in[0..inLen)`, cuyos bytes empiezan en el offset absoluto
    // `inStartOffset` dentro del archivo. Escribe codepoints en `out` y, si
    // `outOffsets` no es nullptr, el offset de inicio de cada uno.
    // Contrato: outCap >= inLen (cada byte produce como mucho un codepoint).
    //   `outLen` = codepoints escritos.
    // Devuelve ErrInvalidArg si outCap < inLen; Ok en caso normal.
    Result decode(const uint8_t* in, size_t inLen, uint64_t inStartOffset,
                  uint32_t* out, uint64_t* outOffsets, size_t outCap,
                  size_t& outLen);

    // Cierra el stream: si quedó una secuencia truncada, emite un U+FFFD.
    // Devuelve el nº de codepoints escritos (0 o 1). `outOffset` recibe el
    // offset del byte truncado si se emite algo y no es nullptr.
    size_t finish(uint32_t* out, uint64_t* outOffsets, size_t outCap);

private:
    // Emite un codepoint ya decodificado aplicando el salto de BOM y la
    // normalización CR/LF. `off` es el offset de inicio en el archivo.
    void emit(uint32_t cp, uint64_t off, uint32_t* out, uint64_t* outOffsets,
              size_t& outLen);

    uint8_t  seqRemaining_;    // Bytes de continuación que faltan (0 = espera lead).
    uint32_t seqAcc_;          // Valor acumulado de la secuencia en curso.
    uint32_t seqMin_;          // Valor mínimo válido (detección de overlong).
    uint64_t seqStartOffset_;  // Offset del primer byte de la secuencia en curso.
    bool     sawCr_;           // El último codepoint real fue un CR (para tragar el LF).
    bool     atStart_;         // Aún no se ha emitido ningún codepoint (para el BOM).
};

}  // namespace ereader
