// config.hpp — Límites y tamaños máximos del núcleo.
//
// Regla §6: nada de asignaciones dinámicas por página. Los buffers se
// dimensionan con estas constantes y se reservan al abrir el libro o al
// arrancar. Ajustar aquí según la geometría/fuente máximas soportadas.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ereader {
namespace cfg {

// Tamaño del bloque de lectura de la SD/archivo (regla §6: leer por bloques).
constexpr size_t kReadBlockBytes = 4096;

// Tamaño del bloque que decodifica Utf8Source de una vez. Más pequeño que el
// bloque de lectura para acotar los buffers de codepoints/offsets en RAM
// (prioridad §5.3). La capa SD puede hacer su propio buffering mayor.
constexpr size_t kDecodeChunkBytes = 1024;

// Nº de bytes iniciales usados para calcular el BookId (CRC32 + tamaño).
constexpr size_t kBookIdPrefixBytes = 4096;

// Máximo de codepoints (glifos) por línea maquetada.
constexpr size_t kMaxGlyphsPerLine = 160;

// Máximo de líneas por página.
constexpr size_t kMaxLinesPerPage = 64;

// Máximo de bytes de una secuencia UTF-8 válida (RFC 3629: 4).
constexpr size_t kMaxUtf8Bytes = 4;

// Codepoint de reemplazo para secuencias inválidas (U+FFFD).
constexpr uint32_t kReplacementChar = 0xFFFD;

// Límite de longitud de rutas en las abstracciones de archivo.
constexpr size_t kMaxPathLen = 128;

// Número máximo de índices de página que se conservan por libro (§7.2 PageIndex).
constexpr size_t kMaxIndicesPerBook = 3;

// Ventanas de escaneo de Gutenberg (acotan la E/S al abrir, §11 <500 ms).
constexpr uint64_t kGutenHeaderWindow = 128 * 1024;  // Buscar START + metadatos.
constexpr uint64_t kGutenTailWindow = 128 * 1024;    // Buscar END cerca del final.

// Longitud máxima de una línea que se analiza al escanear (cabecera/marcadores).
// Las líneas más largas se truncan solo para la comparación; los offsets
// siguen siendo correctos.
constexpr size_t kScanLineMax = 512;

}  // namespace cfg
}  // namespace ereader
