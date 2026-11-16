# Cableado

## DevKit V1 + módulo microSD (SPI) — PROPUESTA, pendiente de confirmar (M2)

Antes de flashear por primera vez hay que **confirmar con el usuario**:
1. El **módulo microSD**: ¿de 5 V (con regulador y conversor de niveles) o solo
   de 3,3 V? De ello depende a qué pin de alimentación se conecta.
2. El **cableado** exacto (abajo).

Bus por defecto: **VSPI**.

| Señal SD | GPIO DevKit V1 |
|----------|----------------|
| CS   | GPIO5  |
| SCK  | GPIO18 |
| MISO | GPIO19 |
| MOSI | GPIO23 |
| VCC  | 5V *o* 3V3 (según módulo — **confirmar**) |
| GND  | GND |

Notas de hardware:
- No usar GPIO6–11 (flash) ni GPIO12 (strapping; puede impedir el arranque).
- GPIO34–39 son solo entrada.
- Frecuencia SPI: empezar a **20 MHz**, reintentar a menor frecuencia si falla el
  montaje, y **registrar la frecuencia final** (se mostrará con `info`).
- Tarjeta: **FAT32**, ≤ 32 GB.

Estructura esperada en la SD:
```
/books/*.txt         libros
/sys/<bookId>/...    índices, progreso y cachés (nunca en flash interna)
```

> Esta tabla se dará por válida cuando el usuario confirme el módulo y el
> cableado. Si cambia algún pin, actualizar aquí y en el código de M2.
