"""Carga el firmware incluido en un Arduino Uno R3 fisico desde Linux."""

import argparse
import hashlib
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys


def puertos_disponibles():
    estables = sorted(Path('/dev/serial/by-id').glob('*'))
    dispositivos = sorted(Path('/dev').glob('ttyACM*')) + sorted(Path('/dev').glob('ttyUSB*'))
    destinos = {puerto.resolve() for puerto in estables}
    return estables + [puerto for puerto in dispositivos if puerto.resolve() not in destinos]


def herramientas(ruta=None, configuracion=None):
    if ruta:
        ejecutable = Path(ruta).expanduser().resolve()
    elif instalado := shutil.which('avrdude'):
        ejecutable = Path(instalado).resolve()
    else:
        soporte = Path.home() / 'Documents/MATLAB/SupportPackages'
        candidatos = sorted(soporte.glob(
            '*/3P.instrset/arduinoide.instrset/aCLI/data/packages/arduino/'
            'tools/avrdude/*/bin/avrdude'
        ))
        if len(candidatos) != 1:
            raise ValueError('No hay un avrdude unico disponible. Indica su ruta con --avrdude.')
        ejecutable = candidatos[0].resolve()
    if not ejecutable.is_file() or not os.access(ejecutable, os.X_OK):
        raise ValueError(f'avrdude no es un archivo ejecutable: {ejecutable}')
    archivo_config = None
    if configuracion:
        archivo_config = Path(configuracion).expanduser().resolve()
    else:
        vecino = ejecutable.parent.parent / 'etc/avrdude.conf'
        if vecino.is_file():
            archivo_config = vecino
    if archivo_config is not None and not archivo_config.is_file():
        raise ValueError(f'No existe la configuracion: {archivo_config}')
    return ejecutable, archivo_config


def validar_puerto(nombre):
    puerto = Path(nombre).expanduser().resolve()
    if not re.fullmatch(r'/dev/tty(?:ACM|USB)[0-9]+', str(puerto)) or not puerto.is_char_device():
        raise ValueError('Indica un puerto USB serie existente: /dev/ttyACM*, /dev/ttyUSB* '
                         'o su enlace en /dev/serial/by-id/. Usa --listar-puertos.')
    if not os.access(puerto, os.R_OK | os.W_OK):
        raise ValueError(f'Sin permisos de lectura/escritura en {puerto}. Revisa el grupo '
                         'del dispositivo y tu pertenencia a el (habitualmente dialout); '
                         'vuelve a iniciar sesion tras un cambio. No ejecutes la carga con sudo.')
    return puerto


def preparar_carga(opciones):
    firmware = (opciones.firmware.expanduser().resolve() if opciones.firmware else
                Path(__file__).resolve().parent.parent / 'firmware/pot_led.hex')
    if firmware.suffix.lower() != '.hex':
        raise ValueError('Selecciona un archivo Intel HEX (.hex), no el ELF ni el modelo.')
    contenido = firmware.read_bytes()
    if not contenido:
        raise ValueError(f'Firmware vacio: {firmware}')
    ejecutable, configuracion = herramientas(opciones.avrdude, opciones.config)
    puerto = validar_puerto(opciones.puerto)
    comando = [str(ejecutable)]
    if configuracion:
        comando += ['-C', str(configuracion)]
    comando += ['-p', 'atmega328p', '-c', 'arduino', '-P', str(puerto),
                '-b', '115200', '-D', '-U', f'flash:w:{firmware}:i']
    print('Destino: Arduino Uno R3 / ATmega328P, bootloader del Uno, 115200 baudios.')
    print(f'Puerto: {puerto}\nFirmware: {firmware}')
    print(f'SHA-256: {hashlib.sha256(contenido).hexdigest()}')
    print('Comando: ' + shlex.join(comando))
    return comando


def main(argumentos=None, confirmar=input):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--listar-puertos', action='store_true', help='listar puertos sin abrirlos')
    parser.add_argument('--puerto', help='puerto del Uno elegido por el usuario; no se selecciona automaticamente')
    parser.add_argument('--firmware', type=Path,
                        help='HEX que se cargara; por defecto el pot_led.hex original del repositorio')
    parser.add_argument('--comprobar', action='store_true', help='comprobar archivos y permisos sin abrir el puerto')
    parser.add_argument('--avrdude', help='ruta al ejecutable; por defecto PATH o soporte Arduino de MATLAB')
    parser.add_argument('--config', help='ruta a avrdude.conf si no esta junto al cargador ni en su ruta predeterminada')
    opciones = parser.parse_args(argumentos)
    try:
        if platform.system() != 'Linux':
            raise ValueError('La carga fisica con este asistente requiere Linux.')
        if opciones.listar_puertos:
            puertos = puertos_disponibles()
            print('\n'.join(map(str, puertos)) if puertos else
                  'No se detectaron puertos USB serie. Conecta el Uno con un cable USB de datos.')
            print('La lista no identifica la placa. Compara antes y despues de conectar el Uno.')
            return 0
        if not opciones.puerto:
            raise ValueError('Debes elegir --puerto. Consulta primero --listar-puertos.')
        comando = preparar_carga(opciones)
        if opciones.comprobar:
            print('PREPARADO: archivos y permisos comprobados. No se abrio el puerto; '
                  'la comunicacion, la placa y el circuito aun no estan verificados.')
            return 0
        print('Se reemplazara el programa de la placa elegida. Confirma que es un Uno R3 '
              'y que puedes sobrescribirlo. Cierra monitores serie y desconecta otros dispositivos del puerto.')
        if confirmar('Escribe CARGAR para continuar: ').strip() != 'CARGAR':
            print('Cancelado; no se abrio el puerto ni se escribio la placa.')
            return 0
        subprocess.run(comando, check=True, timeout=60)
        print('CARGADO: avrdude termino la escritura y verificacion de flash. '
              'Falta comprobar el LED con el potenciometro y repetir tras reiniciar.')
        return 0
    except (ValueError, OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print(f'NO LISTO: {error}', file=sys.stderr)
        print('No se confirma una carga correcta. Si ya comenzo la escritura, la placa '
              'puede necesitar otra carga. Revisa cable, puerto, permisos y monitor serie.', file=sys.stderr)
        return 1
    except (EOFError, KeyboardInterrupt):
        print('\nOperacion interrumpida. Si la escritura habia comenzado, repite la carga '
              'antes de probar el circuito.', file=sys.stderr)
        return 130


if __name__ == '__main__':
    sys.exit(main())