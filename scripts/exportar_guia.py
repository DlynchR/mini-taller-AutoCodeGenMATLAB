"""Exporta la guia Markdown a PDF con Python-Markdown 3.7 y Google Chrome.

La instalacion de la dependencia y el uso estan descritos en la propia guia.
No usa el perfil personal del navegador ni ejecuta MATLAB o el firmware.
"""

from html import escape
from pathlib import Path
import shutil
import subprocess
import tempfile

import markdown


def exportar_guia():
    # Mantener el Markdown como fuente y el HTML intermedio fuera de Git.
    raiz = Path(__file__).resolve().parent.parent
    fuente = raiz / "docs" / "guia-provisional.md"
    salida = fuente.with_suffix(".pdf")
    carpeta_build = raiz / "build"
    navegador = shutil.which("google-chrome")
    if navegador is None:
        raise RuntimeError("Se requiere google-chrome para exportar esta guia.")

    # Un parser Markdown existente interpreta tablas, enlaces y bloques de codigo.
    contenido = markdown.markdown(
        fuente.read_text(encoding="utf-8"),
        extensions=["tables", "fenced_code"],
    )
    carpeta_build.mkdir(exist_ok=True)
    archivo_html = carpeta_build / "guia-provisional.html"

    # La base conserva los enlaces relativos a docs. El estilo solo afecta
    # la impresion: tablas repetibles, codigo envuelto y titulos con su parrafo.
    documento = """<!doctype html>
<html lang="es">
<head>
<meta charset="utf-8">
<title>Guia provisional - Simulink y Wokwi</title>
<base href="BASE_DOCUMENTO">
<style>
@page { size: A4; margin: 17mm 17mm 18mm;
    @bottom-right { content: counter(page) " / " counter(pages);
            font: 8pt "DejaVu Sans", sans-serif; color: #526264; } }
body { font: 10pt/1.4 "DejaVu Serif", serif; color: #202427; }
img { display: block; max-width: 100%; height: auto; break-inside: avoid; }
h1, h2, h3 { font-family: "DejaVu Sans", sans-serif; break-after: avoid; }
h1 { font-size: 22pt; line-height: 1.2; color: #165d60;
     border-bottom: 2pt solid #165d60; padding-bottom: 4mm; }
h2 { font-size: 14pt; margin: 7mm 0 3mm; color: #165d60; }
h3 { font-size: 11pt; margin: 5mm 0 2mm; }
p { margin: 0 0 3mm; orphans: 3; widows: 3; }
li { margin-bottom: 1.5mm; }
ul, ol { padding-left: 6mm; }
a { color: #165d60; text-decoration: none; overflow-wrap: anywhere; }
code, pre { font-family: "DejaVu Sans Mono", monospace; }
code { font-size: 8pt; overflow-wrap: anywhere; }
pre { font-size: 8pt; line-height: 1.35; white-space: pre-wrap;
      overflow-wrap: anywhere; border-left: 2pt solid #87999a;
      background: #f4f6f6; padding: 3mm; break-inside: avoid; }
table { width: 100%; border-collapse: collapse; margin: 3mm 0 4mm;
        table-layout: fixed; font-size: 8.5pt; }
th, td { border: 0.5pt solid #c5cdcd; padding: 2mm;
         text-align: left; vertical-align: top; overflow-wrap: anywhere; }
th { background: #edf3f3; font-family: "DejaVu Sans", sans-serif; }
thead { display: table-header-group; }
tr { break-inside: avoid; }
</style>
</head>
<body>CONTENIDO_DOCUMENTO</body>
</html>
"""
    documento = documento.replace(
        "BASE_DOCUMENTO", escape(fuente.parent.as_uri() + "/", quote=True)
    ).replace("CONTENIDO_DOCUMENTO", contenido)
    archivo_html.write_text(documento, encoding="utf-8")

    # Imprimir con un perfil temporal evita tocar sesiones y credenciales.
    # Crear primero un PDF temporal impide conservar una salida parcial como final.
    with tempfile.TemporaryDirectory(prefix="minitaller-pdf-") as temporal:
        pdf_temporal = Path(temporal) / salida.name
        proceso = subprocess.run(
            [
                navegador,
                "--headless",
                "--disable-gpu",
                "--no-pdf-header-footer",
                f"--user-data-dir={Path(temporal) / 'perfil'}",
                f"--print-to-pdf={pdf_temporal}",
                archivo_html.as_uri(),
            ],
            capture_output=True,
            text=True,
            timeout=90,
        )
        if proceso.returncode != 0 or not pdf_temporal.is_file():
            raise RuntimeError("No se genero el PDF:\n" + proceso.stderr)
        shutil.copyfile(pdf_temporal, salida)
    print(f"PDF generado: {salida}")


if __name__ == "__main__":
    exportar_guia()