"""Exportacion y compilacion reproducible del firmware, sin acceso a hardware."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import xml.etree.ElementTree as ET
import zipfile


DEPENDENCIAS = {
    'core': {
        'version': '1.8.3',
        'url': 'https://downloads.arduino.cc/cores/avr-1.8.3.tar.bz2',
        'sha256': 'de8a9b982477762d3d3e52fc2b682cdd8ff194dc3f1d46f4debdea6a01b33c14',
    },
    'gcc': {
        'version': '7.3.0-atmel3.6.1-arduino7',
        'url': 'https://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2',
        'sha256': 'bd8c37f6952a2130ac9ee32c53f6a660feb79bee8353c8e289eb60fdcefed91e',
    },
}
EXTENSIONES = {'.c', '.cpp', '.h'}


def sha256(archivo):
    return hashlib.sha256(Path(archivo).read_bytes()).hexdigest()


def escribir_json(archivo, datos):
    Path(archivo).write_text(json.dumps(datos, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def exportar(raiz, paquete, metadatos, modelo_copia, referencia):
    modelo = raiz / 'model/pot_led.slx'
    if sha256(modelo) != sha256(modelo_copia):
        raise ValueError('El modelo cambio durante la exportacion; vuelve a generarla.')
    datos = json.loads(Path(metadatos).read_text(encoding='utf-8'))
    destino = raiz / 'generated_code'
    destino.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='exportacion-ci-') as temporal:
        fuentes = Path(temporal) / 'src'
        fuentes.mkdir()
        origenes = {}
        with zipfile.ZipFile(paquete) as archivo:
            for entrada in archivo.infolist():
                nombre = Path(entrada.filename).name
                if Path(nombre).suffix not in EXTENSIONES or entrada.is_dir():
                    continue
                if nombre in origenes:
                    raise ValueError(f'Nombre de fuente ambiguo: {nombre}')
                (fuentes / nombre).write_bytes(archivo.read(entrada))
                origenes[nombre] = entrada.filename
        nombres = datos['sources']
        if not nombres or len(nombres) != len(set(nombres)):
            raise ValueError('Lista de fuentes vacia o duplicada.')
        for nombre in nombres:
            if nombre not in origenes:
                raise ValueError(f'Fuente ausente en packNGo: {nombre}')
        manifiesto = {
            'schema': 1,
            'model_sha256': sha256(modelo),
            'matlab_release': datos['matlab_release'],
            'defines': datos['defines'],
            'sources': nombres,
            'dependencies': DEPENDENCIAS,
            'reference_hex_sha256': sha256(referencia),
            'files': {nombre: sha256(fuentes / nombre) for nombre in sorted(origenes)},
        }
        if (destino / 'src').exists():
            shutil.rmtree(destino / 'src')
        shutil.copytree(fuentes, destino / 'src')
        escribir_json(destino / 'manifest.json', manifiesto)
    verificar(raiz)
    print(f'Exportadas {len(origenes)} fuentes/cabeceras. No se copiaron binarios ni bibliotecas precompiladas.')


def verificar(raiz):
    carpeta = raiz / 'generated_code'
    manifiesto = json.loads((carpeta / 'manifest.json').read_text(encoding='utf-8'))
    if manifiesto['schema'] != 1 or manifiesto['dependencies'] != DEPENDENCIAS:
        raise ValueError('Formato o versiones de dependencias no admitidos; regenera la exportacion.')
    if sha256(raiz / 'model/pot_led.slx') != manifiesto['model_sha256']:
        raise ValueError('El modelo cambio sin regenerar generated_code. Ejecuta exportar_ci en MATLAB.')
    archivos = manifiesto['files']
    presentes = {archivo.relative_to(carpeta / 'src').as_posix()
                 for archivo in (carpeta / 'src').rglob('*') if archivo.is_file()}
    if not archivos or presentes != set(archivos):
        raise ValueError('Inventario de fuentes distinto del manifiesto; regenera la exportacion.')
    for nombre, esperado in archivos.items():
        ruta = carpeta / 'src' / nombre
        if Path(nombre).name != nombre or ruta.is_symlink() or sha256(ruta) != esperado:
            raise ValueError(f'Fuente modificada o ruta no admitida: {nombre}')
    fuentes = manifiesto['sources']
    if not fuentes or len(fuentes) != len(set(fuentes)) or set(fuentes) != {
        nombre for nombre in archivos if Path(nombre).suffix in ('.c', '.cpp')
    }:
        raise ValueError('Lista de unidades de compilacion incompleta o duplicada.')
    if not isinstance(manifiesto['defines'], list) or not all(
        isinstance(valor, str) and valor.startswith('-D') and '\n' not in valor
        for valor in manifiesto['defines']
    ):
        raise ValueError('Definiciones de compilacion invalidas.')
    return manifiesto


def instalar_herramientas(carpeta):
    carpeta.mkdir(parents=True, exist_ok=True)
    for nombre, datos in DEPENDENCIAS.items():
        destino = carpeta / nombre
        sello = destino / '.ci-sha256'
        if sello.is_file() and sello.read_text().strip() == datos['sha256']:
            continue
        if destino.exists():
            raise ValueError(f'Destino de herramientas existente sin sello valido: {destino}')
        with tempfile.TemporaryDirectory(prefix='descarga-ci-', dir=carpeta) as temporal:
            archivo = Path(temporal) / 'paquete.tar.bz2'
            print(f'Descargando {nombre} {datos["version"]}...', flush=True)
            with urllib.request.urlopen(datos['url'], timeout=60) as respuesta, archivo.open('wb') as salida:
                shutil.copyfileobj(respuesta, salida)
            if sha256(archivo) != datos['sha256']:
                raise ValueError(f'SHA-256 incorrecto para {nombre}; no se extrae el paquete.')
            extraido = Path(temporal) / 'extraido'
            extraido.mkdir()
            with tarfile.open(archivo) as paquete:
                paquete.extractall(extraido, filter='data')
            contenido = list(extraido.iterdir())
            if len(contenido) != 1 or not contenido[0].is_dir():
                raise ValueError(f'Estructura inesperada en {nombre}.')
            shutil.move(str(contenido[0]), destino)
            sello.write_text(datos['sha256'] + '\n')
    return carpeta / 'gcc/bin', carpeta / 'core'


def medir(binario, ejecutable_size):
    resultado = subprocess.run([str(ejecutable_size), '--format=berkeley', str(binario)],
                               check=True, capture_output=True, text=True)
    texto, datos, bss = map(int, resultado.stdout.splitlines()[-1].split()[:3])
    memoria = {'flash_bytes': texto + datos, 'ram_static_bytes': datos + bss,
               'flash_limit': 32256, 'ram_limit': 2048}
    if memoria['flash_bytes'] > memoria['flash_limit'] or memoria['ram_static_bytes'] > memoria['ram_limit']:
        raise ValueError(f'Firmware excede la memoria del Uno: {memoria}')
    return memoria


def compilar(raiz, herramientas):
    manifiesto = verificar(raiz)
    binarios, core = instalar_herramientas(herramientas)
    destino = raiz / 'build/ci-firmware'
    destino.mkdir(parents=True, exist_ok=True)
    for nombre in ('pot_led-ci.hex', 'pot_led-ci.elf', 'resumen.json', 'compile_commands.json',
                   'fuentes-reconstruccion.zip'):
        (destino / nombre).unlink(missing_ok=True)
    fuentes = raiz / 'generated_code/src'
    arquitectura = ['-mmcu=atmega328p', '-DF_CPU=16000000L', '-DARDUINO=10801',
                    '-DARDUINO_AVR_UNO', '-DARDUINO_ARCH_AVR', '-D_RUNONTARGETHARDWARE_BUILD_']
    opciones = ['-Os', '-ffunction-sections', '-fdata-sections', *arquitectura,
                *manifiesto['defines'], '-I' + str(fuentes),
                '-I' + str(core / 'cores/arduino'), '-I' + str(core / 'variants/standard')]
    comandos = []
    with tempfile.TemporaryDirectory(prefix='objetos-', dir=destino) as temporal:
        objetos_modelo, objetos_core = [], []
        unidades = [(fuentes / nombre, objetos_modelo) for nombre in manifiesto['sources']]
        unidades += [(archivo, objetos_core) for archivo in sorted((core / 'cores/arduino').iterdir())
                     if archivo.suffix in ('.c', '.cpp', '.S') and archivo.name != 'main.cpp']
        for indice, (fuente, objetos) in enumerate(unidades):
            objeto = Path(temporal) / f'{indice}_{fuente.name}.o'
            if fuente.suffix == '.cpp':
                compilador = binarios / 'avr-g++'
                lenguaje = ['-std=gnu++11', '-fpermissive', '-fno-exceptions', '-fno-threadsafe-statics']
            else:
                compilador = binarios / 'avr-gcc'
                lenguaje = ['-x', 'assembler-with-cpp'] if fuente.suffix == '.S' else ['-std=gnu11']
            comando = [str(compilador), *lenguaje, *opciones, '-c', str(fuente), '-o', str(objeto)]
            subprocess.run(comando, cwd=temporal, check=True)
            objetos.append(str(objeto))
            if objetos is objetos_modelo:
                comandos.append({'directory': str(raiz), 'file': str(fuente), 'arguments': comando})
        biblioteca = Path(temporal) / 'libcore.a'
        subprocess.run([str(binarios / 'avr-ar'), 'rcs', str(biblioteca), *objetos_core], check=True)
        elf = destino / 'pot_led-ci.elf'
        hex_file = destino / 'pot_led-ci.hex'
        subprocess.run([str(binarios / 'avr-gcc'), '-Os', '-mmcu=atmega328p',
                        '-Wl,--gc-sections,--relax', *objetos_modelo, '-lm',
                        str(biblioteca), '-o', str(elf)], check=True)
        subprocess.run([str(binarios / 'avr-objcopy'), '-O', 'ihex', '-R', '.eeprom',
                        str(elf), str(hex_file)], check=True)
    resumen = medir(elf, binarios / 'avr-size')
    resumen.update({'hex_sha256': sha256(hex_file), 'elf_sha256': sha256(elf),
                    'model_sha256': manifiesto['model_sha256'],
                    'matches_matlab_hex': sha256(hex_file) == manifiesto['reference_hex_sha256'],
                    'dependencies': DEPENDENCIAS})
    escribir_json(destino / 'compile_commands.json', comandos)
    escribir_json(destino / 'resumen.json', resumen)
    with zipfile.ZipFile(destino / 'fuentes-reconstruccion.zip', 'w', zipfile.ZIP_DEFLATED) as archivo:
        for fuente in sorted((raiz / 'generated_code').rglob('*')):
            if fuente.is_file():
                archivo.write(fuente, fuente.relative_to(raiz).as_posix())
        for nombre in ('scripts/ci_firmware.py', 'scripts/avr-platform.xml', 'model/pot_led.slx'):
            archivo.write(raiz / nombre, nombre)
        for fuente in sorted(core.rglob('*')):
            if fuente.is_file():
                archivo.write(fuente, 'build/ci-tools/core/' + fuente.relative_to(core).as_posix())
    print(json.dumps(resumen, indent=2))
    return destino


def analizar(raiz, cppcheck):
    manifiesto = verificar(raiz)
    destino = raiz / 'build/ci-firmware'
    base = destino / 'compile_commands.json'
    if not base.is_file():
        raise ValueError('Ejecuta compilar antes de analizar para crear compile_commands.json.')
    resultado = subprocess.run(
        [str(cppcheck), '--project=' + str(base), '--enable=warning,performance,portability',
         '--error-exitcode=1', '--platform=' + str(raiz / 'scripts/avr-platform.xml'),
         '--suppress=missingIncludeSystem', '--xml', '--xml-version=2', '--quiet'],
        cwd=raiz, capture_output=True, text=True,
    )
    (destino / 'cppcheck.xml').write_text(resultado.stderr, encoding='utf-8')
    if resultado.returncode not in (0, 1):
        raise ValueError(f'cppcheck no pudo ejecutarse: {resultado.stdout}\n{resultado.stderr}')
    try:
        reporte = ET.fromstring(resultado.stderr)
    except ET.ParseError as error:
        raise ValueError(f'cppcheck no produjo un informe XML valido: {resultado.stdout}') from error
    diagnosticos = reporte.findall('./errors/error')
    modelo = {nombre for nombre in manifiesto['files']
              if nombre.startswith(('pot_led', 'rt')) or nombre == 'ert_main.c'}
    bloqueantes = []
    for diagnostico in diagnosticos:
        ubicaciones = diagnostico.findall('location')
        es_modelo = any(Path(ubicacion.get('file', '')).name in modelo for ubicacion in ubicaciones)
        severidad = diagnostico.get('severity')
        if severidad == 'error' or (es_modelo and severidad in ('warning', 'performance', 'portability')):
            bloqueantes.append(diagnostico)
        print(f'cppcheck [{severidad}] {diagnostico.get("id")}: {diagnostico.get("msg")}')
    if bloqueantes or (resultado.returncode != 0 and not diagnosticos):
        raise ValueError(f'Analisis estatico rechazado: {len(bloqueantes)} diagnosticos bloqueantes.')
    print(f'OK: cppcheck sin errores bloqueantes; {len(diagnosticos)} avisos registrados en cppcheck.xml.')


def main(argumentos=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('accion', choices=('exportar', 'verificar', 'compilar', 'analizar'))
    parser.add_argument('--raiz', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--herramientas', type=Path)
    parser.add_argument('--paquete', type=Path)
    parser.add_argument('--metadatos', type=Path)
    parser.add_argument('--modelo-copia', type=Path)
    parser.add_argument('--referencia', type=Path)
    parser.add_argument('--cppcheck', default='cppcheck')
    opciones = parser.parse_args(argumentos)
    raiz = opciones.raiz.resolve()
    try:
        if opciones.accion == 'exportar':
            if not all((opciones.paquete, opciones.metadatos, opciones.modelo_copia, opciones.referencia)):
                raise ValueError('Exportar requiere paquete, metadatos, copia del modelo y HEX de referencia.')
            exportar(raiz, opciones.paquete, opciones.metadatos, opciones.modelo_copia, opciones.referencia)
        elif opciones.accion == 'verificar':
            verificar(raiz)
            print('OK: fuentes, modelo y versiones coinciden con el manifiesto.')
        elif opciones.accion == 'analizar':
            analizar(raiz, opciones.cppcheck)
        else:
            compilar(raiz, (opciones.herramientas or raiz / 'build/ci-tools').resolve())
        return 0
    except (OSError, ValueError, KeyError, zipfile.BadZipFile, tarfile.TarError,
            subprocess.CalledProcessError) as error:
        print(f'CI ERROR: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())