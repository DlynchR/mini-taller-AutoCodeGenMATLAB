"""Completa una instalacion existente de MATLAB para el tutorial virtual."""

import argparse
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import xml.etree.ElementTree as ET


PRODUCTOS = (
    "Simulink",
    "Simulink_Coder",
    "Embedded_Coder",
    "Simulink_Support_Package_for_Arduino_Hardware",
)


def instalacion(ruta):
    raiz = Path(ruta.strip().strip('"')).expanduser().resolve()
    ejecutable = raiz / "bin" / "matlab"
    if not ejecutable.is_file():
        raise ValueError(f"No se encontro MATLAB en {ejecutable}. Indica la carpeta raiz de la instalacion.")
    try:
        release = ET.parse(raiz / "VersionInfo.xml").getroot().findtext("release", "")
    except (OSError, ET.ParseError) as error:
        raise ValueError("No se pudo leer VersionInfo.xml de la instalacion elegida.") from error
    if not re.fullmatch(r"R20\d{2}[ab]", release):
        raise ValueError(f"Version de MATLAB no reconocida: {release!r}.")
    return raiz, ejecutable, release


def plataforma_mpm():
    sistema, arquitectura = platform.system(), platform.machine().lower()
    if sistema == "Linux" and arquitectura in ("x86_64", "amd64"):
        return "glnxa64", "mpm"
    raise ValueError(f"Este asistente requiere Linux x86-64; se detecto {sistema} {arquitectura}.")


def ejecutar_mpm(comando):
    salida = []
    with subprocess.Popen(comando, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True, encoding="utf-8", errors="replace") as proceso:
        assert proceso.stdout is not None
        try:
            for linea in proceso.stdout:
                print(linea, end="", flush=True)
                salida.append(linea)
            codigo = proceso.wait()
        except BaseException:
            proceso.kill()
            raise
    ya_instalados = any(linea.strip() == "All specified products are already installed."
                       for linea in salida)
    if codigo == 255 and ya_instalados:
        print("Los productos ya estan instalados; se verificara MATLAB y el soporte Arduino.")
    elif codigo != 0:
        raise subprocess.CalledProcessError(codigo, comando, output="".join(salida))


def preparar(ruta, confirmar=input):
    raiz, matlab, release = instalacion(ruta)
    arquitectura, nombre_mpm = plataforma_mpm()
    print(f"MATLAB: {raiz} ({release})")
    print("Se agregaran: " + ", ".join(PRODUCTOS))
    print("Cierra MATLAB y guarda tus archivos antes de continuar.")
    if confirmar("Escribe \"si\" para descargar mpm e instalar en esta carpeta: ").strip().lower() != "si":
        print("Cancelado; no se descargo ni instalo nada.")
        return 0

    url = f"https://www.mathworks.com/mpm/{arquitectura}/mpm"
    with tempfile.TemporaryDirectory(prefix="minitaller-mpm-") as temporal:
        mpm = Path(temporal) / nombre_mpm
        print(f"Descargando MATLAB Package Manager desde {url} ...", flush=True)
        with urllib.request.urlopen(url, timeout=60) as respuesta, mpm.open("wb") as archivo:
            shutil.copyfileobj(respuesta, archivo)
        mpm.chmod(mpm.stat().st_mode | 0o100)
        print("Instalando productos y soporte Arduino; esto puede tardar varios minutos.", flush=True)
        ejecutar_mpm(
            [str(mpm), "install", f"--release={release}", f"--destination={raiz}",
             f"--products={PRODUCTOS[0]}", *PRODUCTOS[1:]],
        )

    print("Comprobando MATLAB y la biblioteca Arduino...", flush=True)
    comprobacion = (
        "addpath(fullfile(pwd, 'scripts')); estado = verificar_entorno(); "
        "assert(estado.simulinkDisponible && estado.simulinkCoderInstalado && "
        "estado.embeddedCoderInstalado, 'Faltan productos de generacion'); "
        "load_system('arduinolib'); "
        "assert(numel(find_system('arduinolib', 'SearchDepth', 2, 'RegExp', 'on', "
        "'Name', 'Analog Input|PWM')) == 2, 'Faltan bloques Arduino'); "
        "close_system('arduinolib', 0); "
        "fprintf('Soporte Arduino disponible.\\n');"
    )
    subprocess.run([str(matlab), "-batch", comprobacion],
                   cwd=Path(__file__).resolve().parent.parent, check=True)
    print("LISTO: dependencias MATLAB verificadas. Continua con el paso 2 de la guia.")
    return 0


def main():
    parser = argparse.ArgumentParser(description="Instala productos y soporte Arduino en tu MATLAB existente.")
    parser.add_argument("--ruta", help="carpeta raiz de MATLAB; si se omite, se solicita en pantalla")
    opciones = parser.parse_args()
    try:
        ruta = opciones.ruta or input("Ruta de instalacion de MATLAB (carpeta que contiene bin): ")
        return preparar(ruta)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"NO LISTO: {error}", file=sys.stderr)
        print("Revisa la ruta, la conexion, los permisos y tu acceso a MathWorks; "
              "puedes repetir el comando sin modificar el modelo.", file=sys.stderr)
        return 1
    except (EOFError, KeyboardInterrupt):
        print("\nPreparacion cancelada.", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())