"""Controles de integridad sin MATLAB, compilador ni acceso a red."""

from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from scripts import ci_firmware as ci


class PruebasCI(unittest.TestCase):
    def setUp(self):
        temporal = tempfile.TemporaryDirectory(prefix='ci con espacios ')
        self.addCleanup(temporal.cleanup)
        self.raiz = Path(temporal.name)
        (self.raiz / 'model').mkdir()
        self.modelo = self.raiz / 'model/pot_led.slx'
        self.modelo.write_bytes(b'modelo de prueba')
        self.copia = self.raiz / 'copia.slx'
        self.copia.write_bytes(self.modelo.read_bytes())
        self.hex = self.raiz / 'referencia.hex'
        self.hex.write_text(':00000001FF\n')
        self.metadatos = self.raiz / 'metadata.json'
        ci.escribir_json(self.metadatos, {'matlab_release': '2026b',
                                         'defines': ['-DMODEL=pot_led'], 'sources': ['pot_led.c']})
        self.paquete = self.raiz / 'paquete.zip'
        with zipfile.ZipFile(self.paquete, 'w') as archivo:
            archivo.writestr('build/pot_led.c', 'int prueba(void) { return 0; }\n')
            archivo.writestr('build/pot_led.h', '#pragma once\n')
            archivo.writestr('build/pot_led.elf', b'no exportar')
            archivo.writestr('build/buildInfo.mat', b'no exportar')

    def exportar(self):
        ci.exportar(self.raiz, self.paquete, self.metadatos, self.copia, self.hex)

    def test_exporta_solo_fuentes_y_verifica(self):
        self.exportar()
        datos = ci.verificar(self.raiz)
        self.assertEqual(set(datos['files']), {'pot_led.c', 'pot_led.h'})
        self.assertEqual(datos['model_sha256'], ci.sha256(self.modelo))

    def test_detecta_modelo_cambiado_durante_build(self):
        self.modelo.write_bytes(b'cambio')
        with self.assertRaisesRegex(ValueError, 'durante la exportacion'):
            self.exportar()

    def test_detecta_modelo_sin_regenerar(self):
        self.exportar()
        self.modelo.write_bytes(b'cambio posterior')
        with self.assertRaisesRegex(ValueError, 'sin regenerar'):
            ci.verificar(self.raiz)

    def test_detecta_fuente_editada(self):
        self.exportar()
        (self.raiz / 'generated_code/src/pot_led.c').write_text('codigo alterado')
        with self.assertRaisesRegex(ValueError, 'Fuente modificada'):
            ci.verificar(self.raiz)

    def test_detecta_archivo_ajeno_o_ausente(self):
        self.exportar()
        fuente = self.raiz / 'generated_code/src/pot_led.c'
        fuente.rename(fuente.with_name('extra.c'))
        with self.assertRaisesRegex(ValueError, 'Inventario'):
            ci.verificar(self.raiz)

    def test_rechaza_nombres_ambiguos(self):
        with zipfile.ZipFile(self.paquete, 'a') as archivo:
            archivo.writestr('otra/pot_led.h', 'distinta')
        with self.assertRaisesRegex(ValueError, 'ambiguo'):
            self.exportar()

    def test_detecta_unidad_de_compilacion_omitida(self):
        self.exportar()
        archivo = self.raiz / 'generated_code/manifest.json'
        datos = json.loads(archivo.read_text())
        datos['sources'] = []
        ci.escribir_json(archivo, datos)
        with self.assertRaisesRegex(ValueError, 'unidades de compilacion'):
            ci.verificar(self.raiz)

    def test_fallo_integridad_no_descarga_ni_compila(self):
        self.exportar()
        self.modelo.write_bytes(b'otro modelo')
        with patch.object(ci, 'instalar_herramientas') as instalar, \
             self.assertRaisesRegex(ValueError, 'sin regenerar'):
            ci.compilar(self.raiz, self.raiz / 'herramientas')
        instalar.assert_not_called()

    @patch.object(ci.subprocess, 'run')
    def test_politica_cppcheck(self, ejecutar):
        self.exportar()
        destino = self.raiz / 'build/ci-firmware'
        destino.mkdir(parents=True)
        (destino / 'compile_commands.json').write_text('[]')
        for severidad, fuente, bloquea in (('warning', 'vendor.cpp', False),
                                           ('warning', 'pot_led.c', True),
                                           ('error', 'vendor.cpp', True)):
            with self.subTest(severidad=severidad, fuente=fuente):
                informe = (f'<results><errors><error severity="{severidad}" id="prueba" msg="diagnostico">'
                           f'<location file="{fuente}"/></error></errors></results>')
                ejecutar.return_value = subprocess.CompletedProcess([], 1, '', informe)
                if bloquea:
                    with self.assertRaisesRegex(ValueError, 'rechazado'):
                        ci.analizar(self.raiz, 'cppcheck')
                else:
                    ci.analizar(self.raiz, 'cppcheck')
                self.assertTrue((destino / 'cppcheck.xml').is_file())

    @patch.object(ci.subprocess, 'run')
    def test_limites_flash_y_ram(self, ejecutar):
        for texto, datos, bss, excede in ((4700, 80, 200, False), (32200, 80, 200, True),
                                         (4700, 80, 2000, True)):
            with self.subTest(texto=texto, datos=datos, bss=bss):
                ejecutar.return_value = subprocess.CompletedProcess([], 0, f'text data bss\n{texto} {datos} {bss}\n')
                if excede:
                    with self.assertRaisesRegex(ValueError, 'excede'):
                        ci.medir(Path('firmware.elf'), Path('avr-size'))
                else:
                    self.assertEqual(ci.medir(Path('firmware.elf'), Path('avr-size'))['flash_bytes'], 4780)


@unittest.skipUnless(os.environ.get('MINITALLER_TEST_AVR') == '1',
                     'Pruebas AVR reales: activar MINITALLER_TEST_AVR=1')
class PruebasIntegracionAVR(unittest.TestCase):
    def setUp(self):
        origen = Path(__file__).resolve().parent.parent
        self.herramientas = origen / 'build/ci-tools'
        temporal = tempfile.TemporaryDirectory(prefix='compilacion limpia con espacios ')
        self.addCleanup(temporal.cleanup)
        self.raiz = Path(temporal.name)
        for nombre in ('generated_code', 'model'):
            shutil.copytree(origen / nombre, self.raiz / nombre)
        (self.raiz / 'scripts').mkdir()
        for nombre in ('ci_firmware.py', 'avr-platform.xml'):
            shutil.copyfile(origen / 'scripts' / nombre, self.raiz / 'scripts' / nombre)

    def cambiar_fuente(self, nombre, transformar):
        fuente = self.raiz / 'generated_code/src' / nombre
        fuente.write_text(transformar(fuente.read_text()))
        archivo = self.raiz / 'generated_code/manifest.json'
        datos = json.loads(archivo.read_text())
        datos['files'][nombre] = ci.sha256(fuente)
        ci.escribir_json(archivo, datos)

    def test_build_limpio_sin_matlab_ni_binarios_previos(self):
        destino = ci.compilar(self.raiz, self.herramientas)
        resumen = json.loads((destino / 'resumen.json').read_text())
        self.assertLessEqual(resumen['flash_bytes'], 32256)
        self.assertLessEqual(resumen['ram_static_bytes'], 2048)
        self.assertTrue((destino / 'pot_led-ci.hex').is_file())
        comandos = (destino / 'compile_commands.json').read_text()
        self.assertNotIn('/SupportPackages/', comandos)
        self.assertNotIn('/Matlab/', comandos)
        with zipfile.ZipFile(destino / 'fuentes-reconstruccion.zip') as archivo:
            self.assertIn('generated_code/manifest.json', archivo.namelist())
            self.assertIn('build/ci-tools/core/cores/arduino/Arduino.h', archivo.namelist())

    def test_error_de_compilacion_no_produce_hex(self):
        self.cambiar_fuente('pot_led.c', lambda texto: texto + '\n#error CI_FALLO_COMPILACION\n')
        with self.assertRaises(subprocess.CalledProcessError):
            ci.compilar(self.raiz, self.herramientas)
        self.assertFalse((self.raiz / 'build/ci-firmware/pot_led-ci.hex').exists())

    def test_error_de_enlace_no_produce_hex(self):
        def romper_enlace(texto):
            original = 'void pot_led_step(void)\n{'
            self.assertIn(original, texto)
            return texto.replace(original, 'extern void ci_simbolo_ausente(void);\n'
                                 'void pot_led_step(void)\n{\n  ci_simbolo_ausente();', 1)
        self.cambiar_fuente('pot_led.c', romper_enlace)
        with self.assertRaises(subprocess.CalledProcessError):
            ci.compilar(self.raiz, self.herramientas)
        self.assertFalse((self.raiz / 'build/ci-firmware/pot_led-ci.hex').exists())


if __name__ == '__main__':
    unittest.main()