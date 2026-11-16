# Guía de portado — Sunton ESP32-8048S043C

Objetivo: cuando llegue la placa destino, solo se añade una capa de plataforma
nueva. El núcleo (`lib/ereader_core`) **no se reescribe**. Desde M1, el entorno
`s3_compile_check` ya compila el núcleo para el S3 (qio_opi + PSRAM).

## Hardware
- ESP32-S3-WROOM-1 **N16R8**: 16 MB flash, **8 MB PSRAM octal**.
- Pantalla IPS 4,3" **800×480 RGB565** por bus paralelo de 16 bits, ST7262. El
  framebuffer va en **PSRAM** (800×480×2 = 768 000 B por buffer).
- Táctil **GT911** por I2C (SDA=GPIO19, SCL=GPIO20, dir 0x5D/0x14, RST=GPIO38).
  La interrupción no está conectada de fábrica → leer por **sondeo**.
- Retroiluminación: GPIO2 (PWM).
- microSD SPI: **MOSI=GPIO11, SCLK=GPIO12, MISO=GPIO13, CS=GPIO10**.
- Pines libres: solo GPIO17 y GPIO18, más BOON/BOOT (GPIO0).
- USB-serie: CH340C.

## Interfaces a implementar (nueva `lib/platform_s3`)
1. **`IFileSystem`/`IFile`** sobre la microSD SPI del S3 (pines de arriba).
   Reutilizar el patrón de `platform_devkit` (M2); solo cambian los pines y el
   bus. Mantener el acceso bajo **mutex** (varias tareas comparten la SD).
2. **`FontMetrics`**: cargar las tablas `.erfm` (ya portables) desde la SD o
   embebidas. Para el render real harán falta **glifos 4 bpp**; `gen_metrics.py`
   está diseñado para generarlos en una fase futura (misma cobertura de rangos).
3. **`PageSink`** real: en vez de texto por serie, dibujar los glifos de cada
   `Line` (x/y ya calculados) sobre el framebuffer **RGB565** con antialiasing
   4 bpp. El layout no cambia: ya entrega posiciones en píxeles.
4. **Reloj**: instalar `ereader::g_nowMicros = esp_timer_get_time` (igual que la DevKit).

## Configuración de ESP-IDF / build para la pantalla RGB
- `board_build.arduino.memory_type = qio_opi` y `-DBOARD_HAS_PSRAM` (ya en
  `s3_compile_check`). Para la pantalla real: habilitar PSRAM **octal** a máxima
  frecuencia, ubicar los framebuffers en PSRAM (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`),
  y configurar el periférico **LCD_CAM / esp_lcd RGB panel** (pclk, HSYNC/VSYNC,
  DE, 16 líneas de datos) según el esquema de la 8048S043C. Doble buffer si la
  RAM lo permite (2 × 768 KB en los 8 MB de PSRAM).
- Geometría del núcleo: por defecto vertical 480×800; para esta pantalla
  horizontal nativa 800×480, configurar `Geometry` (es un parámetro, no código).

## Riesgos de portado
- **Escrituras en flash desplazan la imagen RGB**: por eso progreso/índices/cachés
  van SIEMPRE en la SD (ya es así). No escribir en flash durante el refresco.
- Pocos pines libres: planificar táctil (I2C) + SD (SPI) + backlight antes de
  añadir periféricos.
- Ancho de banda PSRAM: el framebuffer y el layout compiten; medir refresco real.

## Qué NO cambia
Pipeline de texto, paginación, determinismo, índice, progreso, formatos binarios,
BookId, reflow, UTF-8: todo vive en el núcleo portable y ya está probado en el PC
y compilado para el S3.
