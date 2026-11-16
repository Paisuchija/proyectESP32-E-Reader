# Arquitectura — Núcleo de lectura

## Capas
```
            +--------------------------------------------------+
  app       |  src/main.cpp (DevKit)  ·  src/cli_main.cpp (PC) |
            +--------------------------------------------------+
  plataforma|  lib/platform_devkit (SD, Serial, WiFi)  [M2+]   |
            |  lib/platform_native  (stdio, chrono, mem)       |
            +--------------------------------------------------+
  núcleo    |  lib/ereader_core  (C++17 puro, portable)        |
            +--------------------------------------------------+
```
El núcleo **no** conoce Arduino, ESP-IDF ni el sistema operativo: habla con las
interfaces `IFile`/`IFileSystem`, un reloj inyectable (`clock.hpp`) y la interfaz
de render `PageSink`. Portar al S3 = implementar esas interfaces, sin tocar el núcleo.

## Pipeline de texto
```
IFile (bytes)
  -> Utf8Source           lee por bloques; decodifica UTF-8 con estado; mapea
                          cada codepoint a su offset de origen en bytes
  -> ReflowFilter         codepoints -> tokens (Glyph/Nbsp/Space/LineBreak/
                          ParaBreak/End); une líneas suaves o preserva saltos
  -> LayoutEngine         corte voraz por píxeles (LineBreaker) + apilado
                          vertical en páginas (Paginator)
  -> Page                 líneas con x/y por glifo + offset e info de párrafo
  -> PageSink             render (hoy texto por serie/stdout; mañana RGB565)
```
`FontMetrics` (ancho de avance por codepoint + métricas verticales) alimenta el
layout: `FixedWidthMetrics` (tests) y `TableMetrics` (tabla .erfm desde Literata).

## Identidad y cachés
- `BookId` = CRC32(primeros 4 KB) + tamaño. Sobrevive al renombrado.
- `layout_hash` = CRC32(geometría + reflow + id de fuente). Si cambia, el índice
  se invalida y se usa/crea otro.
- Cachés por libro en `<sysRoot>/<bookId>/`: `i<layoutHash>.idx`, `progress.prg`
  (+ `.bak`). `sysRoot` = `/sys` en la SD, un dir temporal en el PC.

## Determinismo del paginado (invariante central)
Repaginar desde el offset de una página reproduce esa página y las siguientes,
byte a byte. Claves:
1. Una página empieza siempre en el **primer glifo de una línea**; el reflow y el
   LineBreaker recortan el espacio inicial, así que no hay estado "colgando".
2. El flag `startsParagraph` (sangría) **se almacena en el índice y se inyecta**
   al repaginar; no se deduce del contexto anterior.
3. El apilado vertical arranca siempre en el margen superior y las líneas en
   blanco que caerían al inicio de página se descartan.
4. El BOM solo se salta en el offset 0.
Tests: `test_paginator` (sintético exhaustivo) y `test_realbook` (Don Quijote).

## Navegación
- **Siguiente**: el LayoutEngine ya sabe dónde empieza la página siguiente
  (`nextPageOffset`), barato.
- **Anterior**: historial de páginas visitadas (exacto y barato para ir/volver);
  sin historial, vía índice si está; si no, fallback que pagina desde el inicio
  (se optimizará en M3 con búsqueda hacia atrás por párrafos).
- **% leído**: por bytes mientras el índice se construye; por página/total cuando
  está completo.

## Heurísticas de reflow y sus límites
- Reflow ON: un `\n` simple une con espacio; una línea en blanco (o solo con
  espacios) separa párrafos; colapsa espacios múltiples. La sangría de primera
  línea la pone el Paginator, no los espacios del origen.
- No distingue versos, listas ni encabezados. Para poesía (p. ej. *La Divina
  Comedia*) usar **reflow OFF**, que conserva cada `\n` como salto y las líneas
  en blanco.

## Decisiones que afectan al futuro (fuera de alcance en Fase 1)
- El `PageSink` entrega x/y por glifo: el renderer RGB565 del S3 encaja sin
  cambiar el layout.
- `gen_metrics.py` aísla la extracción por glifo pensando en emitir **bitmaps
  4 bpp** para el S3 en una fase futura.
- EPUB/LVGL/OTA/deep-sleep quedan fuera; el diseño por interfaces no los impide.
