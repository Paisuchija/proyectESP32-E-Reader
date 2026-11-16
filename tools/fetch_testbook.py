#!/usr/bin/env python3
"""Descarga UN libro de Project Gutenberg a testdata/ para pruebas locales.

Uso:  python3 tools/fetch_testbook.py <id>   (p. ej. 2000 = Don Quijote)

Respeta los términos de Gutenberg: descarga puntual de un único archivo, con
User-Agent identificable, sin "harvesting" masivo. NO se versiona el resultado
(ver .gitignore). Es solo para los tests de la CLI/paginador en el PC.
"""
import os
import sys
import urllib.request

UA = "ereader-esp32-testfetch/0.1 (+https://example.invalid; contacto: dev@example.invalid)"


def main() -> int:
    book_id = sys.argv[1] if len(sys.argv) > 1 else "2000"
    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"
    out_dir = os.path.join(os.path.dirname(__file__), "..", "testdata")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"pg{book_id}.txt")
    if os.path.exists(out_path):
        print(f"ya existe: {out_path} ({os.path.getsize(out_path)} bytes)")
        return 0
    print(f"descargando {url} ...")
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=30) as resp:
        data = resp.read()  # bytes crudos, sin decodificar
    with open(out_path, "wb") as f:
        f.write(data)
    print(f"guardado: {out_path} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
