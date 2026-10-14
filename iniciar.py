"""Prepara el tutorial virtual desde cualquier ubicacion, sin ejecutar MATLAB."""

from pathlib import Path
import argparse
import json
import shutil
import subprocess
import sys

if sys.version_info < (3, 11):
    raise SystemExit("Se necesita Python 3.11 o posterior. Actualiza Python y repite.")

import tomllib


class ErrorPreparacion(Exception):
    """Fallo que el participante puede resolver con las indicaciones mostradas."""


def validar_proyecto(raiz):
    """Comprueba configuracion, artefactos y cableado; no prueba la emulacion."""
    raiz = Path(raiz).resolve()
    carpeta = raiz / "wokwi_emulation"
    try:
        with (carpeta / "wokwi.toml").open("rb") as archivo:
            config = tomllib.load(archivo)
        circuito = json.loads((carpeta / "diagram.json").read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise ErrorPreparacion(
            f"No se pudo leer la configuracion Wokwi en {carpeta}. Comprueba que "
            "wokwi.toml y diagram.json esten en esa carpeta. Detalle: " + str(error)
        ) from error

    opciones = config.get("wokwi", {})
    if not isinstance(opciones, dict) or opciones.get("version") != 1:
        raise ErrorPreparacion("wokwi.toml debe contener [wokwi] y version = 1.")
    if not isinstance(circuito, dict) or circuito.get("version") != 1:
        raise ErrorPreparacion("diagram.json debe ser un circuito Wokwi version 1.")

    # Resolver las rutas como Wokwi: relativas al TOML, no al terminal actual.
    for clave in ("firmware", "elf"):
        ruta = opciones.get(clave)
        if not isinstance(ruta, str) or not ruta:
            raise ErrorPreparacion(f"Falta una ruta valida para {clave} en wokwi.toml.")
        destino = (carpeta / ruta).resolve()
        if not destino.is_relative_to(raiz):
            raise ErrorPreparacion(f"La ruta {clave} debe permanecer dentro del proyecto.")
        if not destino.is_file() or destino.stat().st_size == 0:
            raise ErrorPreparacion(
                f"Falta el archivo {clave} o esta vacio: {ruta}. "
                "Extrae el repositorio completo o genera el firmware con MATLAB "
                "siguiendo docs/tutorial-detallado.md."
            )

    partes = circuito.get("parts")
    if not isinstance(partes, list) or not all(isinstance(parte, dict) for parte in partes):
        raise ErrorPreparacion("La lista parts de diagram.json no es valida.")
    identificadores = [parte.get("id") for parte in partes]
    if not all(isinstance(nombre, str) and nombre for nombre in identificadores):
        raise ErrorPreparacion("Cada componente debe tener un id de texto no vacio.")
    if len(set(identificadores)) != len(identificadores):
        raise ErrorPreparacion("Hay componentes con id repetido en diagram.json.")

    # Identificar componentes por tipo permite renombrar sus IDs sin romper la guia.
    roles = {}
    for rol, tipo in (("uno", "wokwi-arduino-uno"), ("pot", "wokwi-potentiometer"),
                      ("res", "wokwi-resistor"), ("led", "wokwi-led")):
        encontrados = [parte["id"] for parte in partes if parte.get("type") == tipo]
        if len(encontrados) != 1:
            raise ErrorPreparacion(f"El ejercicio necesita exactamente un componente {tipo}.")
        roles[rol] = encontrados[0]

    conexiones = circuito.get("connections")
    if not isinstance(conexiones, list) or not all(
        isinstance(cable, list) and len(cable) == 4
        and all(isinstance(extremo, str) for extremo in cable[:2])
        for cable in conexiones
    ):
        raise ErrorPreparacion("La lista connections de diagram.json no es valida.")
    pares = {frozenset(cable[:2]) for cable in conexiones}
    necesarias = (
        (f"{roles['uno']}:5V", f"{roles['pot']}:VCC"),
        (f"{roles['uno']}:GND.1", f"{roles['pot']}:GND"),
        (f"{roles['pot']}:SIG", f"{roles['uno']}:A0"),
        (f"{roles['uno']}:3", f"{roles['res']}:1"),
        (f"{roles['res']}:2", f"{roles['led']}:A"),
        (f"{roles['led']}:C", f"{roles['uno']}:GND.1"),
    )
    for origen, destino in necesarias:
        if frozenset((origen, destino)) not in pares:
            raise ErrorPreparacion(f"Falta la conexion del ejercicio: {origen} -> {destino}.")
    return carpeta


def ejecutar_code(ejecutable, argumentos, timeout=60):
    """Usa argumentos separados para admitir espacios sin construir un shell."""
    try:
        resultado = subprocess.run(
            [ejecutable, *argumentos], capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        raise ErrorPreparacion(
            "VS Code tardo demasiado. Comprueba Internet y las ventanas de VS Code; "
            "despues repite el comando."
        ) from error
    except OSError as error:
        raise ErrorPreparacion(f"No se pudo ejecutar VS Code: {error}") from error
    if resultado.returncode != 0:
        detalle = (resultado.stderr or resultado.stdout).strip()
        raise ErrorPreparacion(
            f"VS Code no completo la operacion (codigo {resultado.returncode}). "
            f"{detalle}\nPuedes instalar Wokwi Simulator desde Extensions y repetir."
        )
    return resultado.stdout


def comprobar_extension(ejecutable):
    extensiones = ejecutar_code(ejecutable, ["--list-extensions"])
    return "wokwi.wokwi-vscode" in {linea.strip().lower() for linea in extensiones.splitlines()}


def preparar_vscode(carpeta, solo_comprobar=False, sin_abrir=False):
    """Instala solo la extension que falta; no gestiona licencias ni cuentas."""
    ejecutable = shutil.which("code")
    if ejecutable is None:
        raise ErrorPreparacion(
            "No se encuentra el comando code. Instala VS Code y abre un terminal nuevo. "
            "Comprueba que el comando code este disponible en PATH."
        )
    print("[2/3] Comprobando la extension Wokwi...")
    if not comprobar_extension(ejecutable):
        if solo_comprobar:
            raise ErrorPreparacion(
                "Falta Wokwi Simulator. Ejecuta este asistente sin --comprobar para instalarlo."
            )
        print("Instalando Wokwi desde el Marketplace; se necesita Internet...", flush=True)
        ejecutar_code(ejecutable, ["--install-extension", "wokwi.wokwi-vscode"], timeout=180)
        if not comprobar_extension(ejecutable):
            raise ErrorPreparacion("Wokwi no aparece instalado. Revisa Extensions en VS Code.")
    if solo_comprobar:
        print("[3/3] Comprobacion terminada. No se instalo nada ni se abrieron ventanas.")
    elif sin_abrir:
        print("[3/3] Preparacion terminada. No se abrieron ventanas (--sin-abrir).")
    else:
        # Wokwi encuentra su TOML en la raiz de esta ventana dedicada al circuito.
        # Los binarios siguen resolviendose con ../firmware, sin copiar archivos.
        ejecutar_code(ejecutable, ["--new-window", "--folder-uri", carpeta.as_uri()])
        print("[3/3] Se solicito abrir una ventana de VS Code dedicada al circuito.")


def main(argumentos=None):
    parser = argparse.ArgumentParser(
        description="Prepara el ejercicio virtual. Requiere Python 3.11+ y VS Code; no MATLAB."
    )
    modos = parser.add_mutually_exclusive_group()
    modos.add_argument("--comprobar", action="store_true",
                       help="solo revisar archivos y extension; no instalar ni abrir ventanas")
    modos.add_argument("--sin-abrir", action="store_true",
                       help="preparar e instalar Wokwi si falta, sin abrir otra ventana")
    opciones = parser.parse_args(argumentos)
    try:
        print("[1/3] Comprobando archivos, firmware y cableado...")
        carpeta = validar_proyecto(Path(__file__).resolve().parent)
        preparar_vscode(carpeta, opciones.comprobar, opciones.sin_abrir)
    except (ErrorPreparacion, OSError) as error:
        print(f"NO LISTO: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nPreparacion interrumpida. Puedes repetir el comando.", file=sys.stderr)
        return 130
    print("\nPREPARADO. La licencia y el funcionamiento del circuito aun requieren comprobacion.")
    if not opciones.comprobar:
        if opciones.sin_abrir:
            print("Repite sin --sin-abrir cuando quieras abrir el circuito.")
        print("En la ventana del circuito:")
        print("  1. Si se pide confianza, confia solo en un repositorio que hayas revisado.")
        print("  2. Si falta activacion: F1 > Wokwi: Request a New License.")
        print("     Completa el acceso en el navegador. No compartas claves ni tokens.")
        print("     Si solicita pago, revisa las condiciones del servicio antes de continuar.")
        print("  3. F1 > Wokwi: Start Simulator.")
        print("  4. Prueba minimo, medio, maximo, regreso al minimo y reinicio.")
        print("El firmware actual no envia UART; el Monitor Serie puede estar vacio.")
    return 0


if __name__ == "__main__":
    sys.exit(main())