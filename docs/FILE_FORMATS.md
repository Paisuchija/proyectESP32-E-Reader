# Formatos binarios

Todos little-endian. CRC32 = IEEE 802.3 reflejado (igual que zlib),
implementado en `lib/ereader_core/src/crc32.cpp`.

## BookId
No es un archivo: es `CRC32(primeros 4096 bytes) : u32` + `tamaño : u64`.
Representación en texto (para nombres de carpeta): `%08x%08x` (crc, tamaño&0xFFFFFFFF).

## Índice de páginas — `i<layoutHash>.idx`  (magic `ERIX`, versión 1)
Cabecera fija de 48 bytes:

| off | tipo | campo |
|----:|------|-------|
| 0   | u8[4]| magic `"ERIX"` |
| 4   | u16  | version (=1) |
| 6   | u16  | flags (bit0 = completo) |
| 8   | u32  | bookCrc |
| 12  | u64  | bookSize |
| 20  | u32  | layoutHash |
| 24  | u64  | bodyStart |
| 32  | u64  | bodyEnd |
| 40  | u32  | pageCount |
| 44  | u32  | crc32 (sobre bytes [0,44) + todas las entradas; válido si completo) |

Entradas (desde el offset 48), `pageCount` × 10 bytes:

| off | tipo | campo |
|----:|------|-------|
| +0  | u64  | offset de origen del primer carácter de la página |
| +8  | u16  | flags (bit0 = la página empieza un párrafo) |

- Construcción incremental: `pageCount` se refresca cada 256 páginas (checkpoint
  de reanudación) y en `finalize()`, que además escribe el CRC y marca completo.
- Reanudar: se reescribe la última entrada (repaginar desde su offset reproduce
  esa misma página), evitando duplicados.
- `layoutHash` distinto o CRC inválido ⇒ el índice se considera no válido y se
  reconstruye.

## Progreso — `progress.prg` (+ `.bak`, `.tmp`)  (magic `ERPG`, versión 1)
Registro fijo de 44 bytes:

| off | tipo | campo |
|----:|------|-------|
| 0   | u8[4]| magic `"ERPG"` |
| 4   | u16  | version (=1) |
| 6   | u16  | flags |
| 8   | u32  | bookCrc |
| 12  | u64  | bookSize |
| 20  | u64  | byteOffset (posición de lectura, en bytes) |
| 28  | u32  | layoutHash (informativo) |
| 32  | u64  | timestampMs |
| 40  | u32  | crc32 (sobre [0,40)) |

Escritura atómica: escribir `.tmp` + `flush`; mover el actual a `.bak`; renombrar
`.tmp` al definitivo. Lectura: probar definitivo → `.bak` → `.tmp`, validando CRC
y BookId. Un corte entre pasos se recupera desde `.bak` (ver `test_progress`).

## Marcadores — `bookmarks.bmk`  (magic `ERBM`, versión 1) — M3
Mismo esquema atómico. Cabecera con magic/version/bookId/count, luego `count`
entradas `{ offset u64, label[…] }`, y CRC32 final. (Se implementa en M3.)

## Métricas de fuente — `metrics_<tag>_<size>.erfm`  (magic `ERFM`, versión 1)
Generado por `tools/gen_metrics.py`. Cabecera 24 bytes:

| off | tipo | campo |
|----:|------|-------|
| 0   | u8[4]| magic `"ERFM"` |
| 4   | u16  | version (=1) |
| 6   | u16  | sizePx |
| 8   | u16  | lineHeight |
| 10  | u16  | ascent |
| 12  | u16  | descent |
| 14  | u16  | replAdvance (ancho de reemplazo para codepoints sin glifo) |
| 16  | u16  | rangeCount |
| 18  | u16  | reservado |
| 20  | u32  | reservado |

Luego `rangeCount` rangos contiguos: `firstCp u32`, `count u16`, `count × u16`
anchos de avance (px). Cobertura: Basic Latin, Latin-1 Supplement, Latin
Extended-A y puntuación general (– — ‘ ’ “ ” …).
