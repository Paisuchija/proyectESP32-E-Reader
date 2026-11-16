# CLAUDE.md — Reglas del proyecto E-reader ESP32

Guía para futuras sesiones de Claude Code en este repositorio. Léela entera
antes de tocar código.

## Qué es esto
Núcleo de lectura de TXT **portable** (C++17) para un e-reader tipo Kindle sobre
ESP32, validado en una **DevKit V1** con microSD y consola serie. El hardware
destino (Sunton ESP32-8048S043C, S3 N16R8) llegará después; cuando llegue solo
se añadirá una capa de pantalla/táctil/SD **sin reescribir el núcleo**.

Fase 1 por hitos: **M1** núcleo en el PC ✅ · **M2** DevKit+SD+consola ·
**M3** segundo plano+robustez · **M4** descarga Gutenberg.

## Estilo
- C++17. Identificadores en **inglés**; comentarios y documentación en **español**.
- Compila con `-Wall -Wextra` y trátalos en serio (0 warnings).
- `lib/ereader_core/` es **puro y portable**: nada de `Arduino.h`, `String`,
  `std::string`, excepciones, ni STL de asignación en rutas calientes. Errores
  con el enum `Result`. Reloj inyectable vía `ereader::g_nowMicros` (clock.hpp).
- **Memoria (crítico)**: nunca cargar un libro en RAM; leer por bloques
  (`cfg::kReadBlockBytes`/`kDecodeChunkBytes`). Buffers de tamaño fijo definidos
  en `config.hpp`; **nada de asignación dinámica por página**.
- **Nada en flash interna**: índices, progreso y cachés van SIEMPRE en la SD
  (en el dispositivo bajo `/sys`; en el PC, un dir temporal). NVS solo para Wi-Fi.

## Invariantes que NO se deben romper
- **Determinismo del paginador**: paginar desde el offset de cualquier página
  debe dar exactamente las mismas páginas que paginar en continuo. Garantizado
  porque una página empieza siempre en el primer glifo de una línea, el reflow
  recorta el espacio inicial, y el flag `startsParagraph` se **inyecta** (no se
  deduce del contexto). Hay test: `test/test_paginator` y `test/test_realbook`.
- **UTF-8 con estado**: un carácter partido en el borde de un bloque no se pierde
  (`Utf8Decoder`). BOM solo se salta en el offset 0.
- **Progreso/índice atómicos y verificados**: `.tmp`→`rename`, copia `.bak`,
  CRC32. Offsets en **bytes**, nunca números de página.

## Comandos
Requiere PlatformIO (`pipx install platformio`) y, para la fuente, un venv:
`python3 -m venv tools/.venv && tools/.venv/bin/pip install fonttools`.

```bash
export PATH="$HOME/.local/bin:$PATH"     # donde pipx deja 'pio'

# Tests del núcleo en el PC (todos deben pasar):
pio test -e native
pio test -e native -f test_utf8          # un solo suite

# Compilar la CLI nativa de paginación:
pio run -e native
.pio/build/native/program testdata/pg2000.txt --font assets/fonts/metrics_lit_22.erfm --index --pages 3

# Comprobar portabilidad al S3 (solo compila el núcleo):
pio run -e s3_compile_check

# DevKit V1 (M2+): compilar / flashear / monitor serie:
pio run -e devkit_v1
pio run -e devkit_v1 -t upload
pio device monitor -b 115200

# Métricas de fuente (.erfm) desde la TTF OFL:
tools/.venv/bin/python tools/gen_metrics.py --ttf assets/fonts/Literata.ttf --sizes 18 22 26 --out assets/fonts
# (modo sin fuente: --synthetic)

# Libro real de prueba (no se versiona):
python3 tools/fetch_testbook.py 2000     # Don Quijote -> testdata/pg2000.txt
```

## Plataforma / versiones (verificado)
- PlatformIO platform: **pioarduino** `55.03.312-1` (Arduino-ESP32 **3.3.12**,
  ESP-IDF **5.5.5**). El platform oficial tardó en dar soporte v3.
- Tests: Unity (`test_framework = unity`), entorno `native`.
- Fuente: **Literata** (OFL; `assets/fonts/OFL.txt`).

## Antes de flashear la DevKit (M2)
Preguntar al usuario el **módulo microSD** (5 V con regulador/level-shifter o
solo 3,3 V) y confirmar el **cableado** (VSPI: CS=5, SCK=18, MISO=19, MOSI=23).
Documentar en `docs/WIRING.md`. Empezar el SPI a 20 MHz con reintento a menor
frecuencia y registrar la final.

## Mediciones
Nunca inventar números. Distinguir **medido** (con el comando/script),
**estimado** y **objetivo**. Las cifras de PC NO son las del dispositivo; los
criterios de §11 son de la DevKit y se miden con `tools/hil.py` (M3). Ver
`docs/MEASUREMENTS.md`.

## Memoria de Claude
Hay memoria persistente en `~/.claude/.../memory/` indexada por `MEMORY.md`.
Guardar ahí decisiones no obvias del proyecto; no duplicar lo que ya está en el
código o en este archivo.
```
