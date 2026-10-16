"""Pruebas de carga sin abrir un puerto ni escribir una placa real."""

from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import Mock, patch

from scripts import cargar_hardware as cargar


class PruebasCarga(unittest.TestCase):
    def setUp(self):
        temporal = tempfile.TemporaryDirectory(prefix='hardware de prueba ')
        self.addCleanup(temporal.cleanup)
        self.raiz = Path(temporal.name)
        (self.raiz / 'firmware').mkdir()
        self.firmware = self.raiz / 'firmware/pot_led.hex'
        self.firmware.write_text(':00000001FF\n')
        self.avrdude = self.raiz / 'cargador con espacios/bin/avrdude'
        self.avrdude.parent.mkdir(parents=True)
        self.avrdude.touch(mode=0o700)
        self.config = self.avrdude.parent.parent / 'etc/avrdude.conf'
        self.config.parent.mkdir()
        self.config.touch()
        self.salida = StringIO()
        self.addCleanup(patch.stopall)
        patch.object(cargar, '__file__', str(self.raiz / 'scripts/cargar_hardware.py')).start()
        patch.object(cargar.platform, 'system', return_value='Linux').start()
        self.puerto = patch.object(cargar, 'validar_puerto', return_value=Path('/dev/ttyACM0')).start()
        self.proceso = patch.object(cargar.subprocess, 'run').start()
        self.argumentos = ['--puerto', '/dev/ttyACM0', '--avrdude', str(self.avrdude)]

    def ejecutar(self, argumentos=None, respuesta='no'):
        self.confirmacion = Mock(return_value=respuesta)
        with redirect_stdout(self.salida), redirect_stderr(self.salida):
            return cargar.main(self.argumentos if argumentos is None else argumentos,
                               confirmar=self.confirmacion)

    def test_comprobar_no_abre_puerto_ni_pide_confirmacion(self):
        self.assertEqual(self.ejecutar(self.argumentos + ['--comprobar']), 0)
        self.proceso.assert_not_called()
        self.confirmacion.assert_not_called()
        self.assertIn('PREPARADO', self.salida.getvalue())
        self.assertIn('SHA-256:', self.salida.getvalue())

    def test_carga_solo_flash_con_verificacion_y_rutas_con_espacios(self):
        self.assertEqual(self.ejecutar(respuesta='CARGAR'), 0)
        self.proceso.assert_called_once_with(
            [str(self.avrdude), '-C', str(self.config), '-p', 'atmega328p',
             '-c', 'arduino', '-P', '/dev/ttyACM0', '-b', '115200', '-D',
             '-U', f'flash:w:{self.firmware}:i'], check=True, timeout=60)
        self.assertIn('CARGADO', self.salida.getvalue())

    def test_cancelacion_no_invoca_cargador(self):
        self.assertEqual(self.ejecutar(), 0)
        self.proceso.assert_not_called()
        self.assertIn('Cancelado', self.salida.getvalue())

    def test_exige_puerto_explicito(self):
        self.assertEqual(self.ejecutar([]), 1)
        self.proceso.assert_not_called()
        self.assertIn('Debes elegir --puerto', self.salida.getvalue())

    def test_sin_firmware_no_carga(self):
        self.firmware.unlink()
        self.assertEqual(self.ejecutar(respuesta='CARGAR'), 1)
        self.proceso.assert_not_called()
        self.confirmacion.assert_not_called()

    def test_firmware_vacio_no_carga(self):
        self.firmware.write_text('')
        self.assertEqual(self.ejecutar(respuesta='CARGAR'), 1)
        self.proceso.assert_not_called()

    def test_puerto_sin_permiso_no_carga(self):
        self.puerto.side_effect = ValueError('Sin permisos')
        self.assertEqual(self.ejecutar(respuesta='CARGAR'), 1)
        self.proceso.assert_not_called()

    def test_fallo_y_timeout_no_anuncian_exito(self):
        for error in (subprocess.CalledProcessError(1, 'avrdude'),
                      subprocess.TimeoutExpired('avrdude', 60)):
            with self.subTest(error=error):
                self.proceso.side_effect = error
                self.assertEqual(self.ejecutar(respuesta='CARGAR'), 1)
                self.assertNotIn('CARGADO:', self.salida.getvalue())

    def test_lista_vacia_no_abre_puertos(self):
        with patch.object(cargar, 'puertos_disponibles', return_value=[]):
            self.assertEqual(self.ejecutar(['--listar-puertos']), 0)
        self.proceso.assert_not_called()
        self.puerto.assert_not_called()
        self.assertIn('No se detectaron', self.salida.getvalue())

    def test_rechaza_otro_sistema(self):
        with patch.object(cargar.platform, 'system', return_value='Windows'):
            self.assertEqual(self.ejecutar(['--listar-puertos']), 1)
        self.proceso.assert_not_called()

    def test_configuracion_inexistente_no_carga(self):
        self.assertEqual(self.ejecutar(self.argumentos + ['--config', str(self.raiz / 'ausente')]), 1)
        self.proceso.assert_not_called()


class PruebasPuerto(unittest.TestCase):
    def test_rechaza_archivo_y_dispositivo_ajeno(self):
        for nombre in ('/dev/null', '/tmp/firmware', '/dev/ttyACM999999'):
            with self.subTest(nombre=nombre), self.assertRaises(ValueError):
                cargar.validar_puerto(nombre)

    @patch.object(Path, 'is_char_device', return_value=True)
    @patch.object(cargar.os, 'access', return_value=False)
    def test_rechaza_permisos_insuficientes(self, acceso, dispositivo):
        with self.assertRaisesRegex(ValueError, 'Sin permisos'):
            cargar.validar_puerto('/dev/ttyACM999999')

    @patch.object(Path, 'is_char_device', return_value=True)
    @patch.object(cargar.os, 'access', return_value=True)
    def test_acepta_usb_serie_con_permisos(self, acceso, dispositivo):
        self.assertEqual(cargar.validar_puerto('/dev/ttyUSB999999'), Path('/dev/ttyUSB999999'))


if __name__ == '__main__':
    unittest.main()