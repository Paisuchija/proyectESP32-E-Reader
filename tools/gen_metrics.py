#!/usr/bin/env python3
"""Genera tablas de métricas (.erfm) para TableMetrics.

Modos:
  - TTF (requiere `fonttools`): extrae el ancho de avance de cada glifo de una
    fuente OFL (p. ej. Literata) y lo escala a px. Cobertura: Basic Latin,
    Latin-1 Supplement, Latin Extended-A y puntuación general.
  - Sintético (sin fuente): modelo de anchos aproximado, suficiente para la CLI
    y los tests cuando la TTF no está disponible.

Uso:
  python3 tools/gen_metrics.py --ttf assets/fonts/Literata.ttf --sizes 18 22 26 \
          --out assets/fonts
  python3 tools/gen_metrics.py --synthetic --sizes 18 22 26 --out assets/fonts

Formato de salida: ver lib/ereader_core/include/ereader/table_metrics.hpp (ERFM).

Diseño pensado para el S3: la función iter_glyphs() aísla la extracción por
glifo; una fase futura añadirá aquí el volcado de bitmaps 4 bpp para el renderer
del ESP32-S3 sin cambiar el resto.
"""
import argparse
import os
import struct
import sys

# Rangos cubiertos: (primer_cp, ultimo_cp_inclusive).
RANGES = [
    (0x20, 0x7E),    # Basic Latin imprimible
    (0xA0, 0xFF),    # Latin-1 Supplement (á é í ó ú ñ ü ¿ ¡ « »)
    (0x100, 0x17F),  # Latin Extended-A
    (0x2010, 0x2026),  # – — ‘ ’ “ ” … (puntuación general)
]

MAGIC = b"ERFM"
VERSION = 1


def synthetic_advance(cp: int, size: int) -> int:
    """Modelo de ancho aproximado (sin fuente real)."""
    narrow = set(ord(c) for c in "iIl.,:;'|!ﬁ ")
    wide = set(ord(c) for c in "mMwW@—–") | {0x2014, 0x2013}
    if cp == 0x20 or cp == 0xA0:
        return max(1, round(size * 0.30))
    if cp in narrow:
        return max(1, round(size * 0.30))
    if cp in wide:
        return max(1, round(size * 0.80))
    return max(1, round(size * 0.52))


def iter_glyphs_ttf(ttf_path):
    """Devuelve (unitsPerEm, dict cp->advanceWidth, (ascent,descent,lineGap))."""
    from fontTools.ttLib import TTFont  # import local: dependencia opcional
    font = TTFont(ttf_path)
    upm = font["head"].unitsPerEm
    cmap = font.getBestCmap()
    hmtx = font["hmtx"]
    adv = {}
    for cp, name in cmap.items():
        try:
            adv[cp] = hmtx[name][0]
        except KeyError:
            pass
    hhea = font["hhea"]
    return upm, adv, (hhea.ascent, -hhea.descent, hhea.lineGap)


def build_table(size: int, ttf=None) -> bytes:
    if ttf:
        upm, adv_units, (asc_u, desc_u, gap_u) = iter_glyphs_ttf(ttf)
        def adv_px(cp):
            u = adv_units.get(cp)
            return None if u is None else max(1, round(u * size / upm))
        ascent = max(1, round(asc_u * size / upm))
        descent = max(1, round(desc_u * size / upm))
        line_height = ascent + descent + max(0, round(gap_u * size / upm))
    else:
        def adv_px(cp):
            return synthetic_advance(cp, size)
        ascent = round(size * 0.78)
        descent = round(size * 0.22)
        line_height = size + max(2, round(size * 0.15))

    repl = max(1, round(size * 0.52))

    # Cabecera.
    header = bytearray()
    header += MAGIC
    header += struct.pack("<H", VERSION)
    header += struct.pack("<H", size)
    header += struct.pack("<H", line_height)
    header += struct.pack("<H", ascent)
    header += struct.pack("<H", descent)
    header += struct.pack("<H", repl)
    header += struct.pack("<H", len(RANGES))
    header += struct.pack("<H", 0)        # reservado
    header += struct.pack("<I", 0)        # reservado

    body = bytearray()
    for first, last in RANGES:
        count = last - first + 1
        body += struct.pack("<I", first)
        body += struct.pack("<H", count)
        for cp in range(first, last + 1):
            a = adv_px(cp)
            if a is None:
                a = repl
            body += struct.pack("<H", min(65535, a))
    return bytes(header) + bytes(body)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ttf", help="ruta a la fuente TTF OFL")
    ap.add_argument("--synthetic", action="store_true", help="sin fuente (modelo aproximado)")
    ap.add_argument("--sizes", nargs="+", type=int, default=[18, 22, 26])
    ap.add_argument("--out", default="assets/fonts")
    args = ap.parse_args()

    if not args.ttf and not args.synthetic:
        print("Indica --ttf <archivo> o --synthetic", file=sys.stderr)
        return 2
    if args.ttf and not os.path.exists(args.ttf):
        print(f"No existe la TTF: {args.ttf}", file=sys.stderr)
        return 2

    os.makedirs(args.out, exist_ok=True)
    tag = "lit" if args.ttf else "syn"
    for size in args.sizes:
        data = build_table(size, ttf=args.ttf)
        path = os.path.join(args.out, f"metrics_{tag}_{size}.erfm")
        with open(path, "wb") as f:
            f.write(data)
        print(f"escrito {path} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
