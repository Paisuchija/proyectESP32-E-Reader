# Mediciones

Distinción obligatoria (§1.7): **medido** (con el comando indicado),
**estimado**, **objetivo**. Las cifras de PC **no** son las del dispositivo.

## Entorno PC (medido)
- Host: macOS (Apple Silicon), Apple clang 21, build `native` (`-O2` por defecto
  del platform native). Reloj: `std::chrono::steady_clock`.
- Libro: `testdata/pg2000.txt` (Don Quijote), **2 225 777 bytes**; cuerpo
  Gutenberg `[904, 2206879)` = 2 205 975 bytes.

Comando:
```
.pio/build/native/program testdata/pg2000.txt --font assets/fonts/metrics_lit_22.erfm --index --pages 1
```

| Métrica (PC) | Valor medido |
|---|---|
| Detección Gutenberg + apertura + página 1 | incluida en el arranque; layout de 1 página ≈ **41 µs** |
| Índice completo (Literata 22 px, geom. 480×800) | **2 755 páginas en ≈ 88,6 ms** |
| Índice completo (ancho fijo 11 px) | **2 322 páginas en ≈ 110,5 ms** |
| Reapertura con índice en caché | índice reutilizado (**0 ms**), progreso restaurado |

> Nota: el PC lee el archivo del sistema de archivos del host, no de una SD SPI.
> En la DevKit el cuello de botella será la SD; estos números solo validan que el
> algoritmo es O(n) y rápido, no predicen el tiempo en hardware.

## Compilación de los entornos (medido)
Con pioarduino `55.03.312-1` (Arduino 3.3.12 / IDF 5.5.5), `-Wall -Wextra`:

| Entorno | Resultado | Tamaño del núcleo (main mínimo) |
|---|---|---|
| `native` | 28/28 tests OK | — |
| `s3_compile_check` (ESP32-S3, qio_opi, PSRAM) | **SUCCESS** | RAM 22 188 B (6,8 %), Flash 285 759 B (8,5 %) |
| `devkit_v1` (ESP32) | **SUCCESS** | binario generado |

> Son tamaños del `main` mínimo (solo enlaza el núcleo), no de la app completa;
> sirven para confirmar la portabilidad al S3 desde M1.

## Objetivos en la DevKit V1 (§11) — PENDIENTE de medir en hardware
Se medirán con `tools/hil.py` (M3) y los comandos `bench`/`stress`/`info` sobre
la placa. Solo **cálculo**, sin contar la impresión por serie, con
`esp_timer_get_time()`.

| Criterio | Objetivo | Estado |
|---|---|---|
| Abrir TXT ~2 MB sin índice y obtener página 1 | < 500 ms | pendiente (M2/M3) |
| Página siguiente/anterior con índice | < 50 ms | pendiente (M3) |
| Índice completo de pg2000.txt | medir y reportar | pendiente (M3) |
| `stress 1000`: heap y bloque libre a ±1 KB del inicio | sin fugas | pendiente (M3) |
| Heap libre mínimo con Wi-Fi apagado | ≥ 40 KB | pendiente (M3) |
| Heap mínimo durante descarga | reportar | pendiente (M4) |
| 0 reinicios en 60 min de `stress` | estabilidad | pendiente (M3) |

## Consumo de RAM del núcleo (estimado)
Buffers fijos por instancia (de `config.hpp`, `page.hpp`):
- `Utf8Source`: bloque 1 KB + codepoints (1024×4) + offsets (1024×8) ≈ **13 KB**.
  La sesión usa una para lectura y otra para indexar (handle propio) ≈ 26 KB.
- `Page`: `kMaxLinesPerPage`(64) × (`kMaxGlyphsPerLine`(160) × 6 B) ≈ **60 KB**.
  Se reutiliza una sola; el indexado usa otra. Ajustar estos máximos a la
  geometría/fuente reales reducirá la RAM; se calibrará con mediciones en M2/M3.

> Estos tamaños son **estimados**; los valores reales de heap se reportarán con
> `info` en la placa. Si no caben con holgura (≥40 KB libres), se bajarán
> `kMaxGlyphsPerLine`/`kMaxLinesPerPage` y `kDecodeChunkBytes`.
