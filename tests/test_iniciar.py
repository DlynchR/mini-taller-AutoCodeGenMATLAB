"""Pruebas del asistente sin instalar extensiones ni abrir el simulador."""

from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import iniciar


class PruebasAsistente(unittest.TestCase):
    def setUp(self):
        self.temporal = tempfile.TemporaryDirectory(prefix="taller pruebas ")
        self.addCleanup(self.temporal.cleanup)
        self.raiz = Path(self.temporal.name) / "proyecto renombrado"
        origen = Path(iniciar.__file__).resolve().parent
        for nombre in ("wokwi_emulation", "firmware"):
            shutil.copytree(origen / nombre, self.raiz / nombre)
        self.carpeta = self.raiz / "wokwi_emulation"
        self.salida = StringIO()
        self.addCleanup(patch.stopall)
        patch("iniciar.__file__", str(self.raiz / "iniciar.py")).start()

    def ejecutar(self, opciones=()):
        with redirect_stdout(self.salida), redirect_stderr(self.salida):
            return iniciar.main(list(opciones))

    def test_rutas_reubicadas_con_espacios(self):
        self.assertEqual(iniciar.validar_proyecto(self.raiz), self.carpeta)

    def test_firmware_faltante(self):
        (self.raiz / "firmware" / "pot_led.hex").unlink()
        with self.assertRaisesRegex(iniciar.ErrorPreparacion, "Extrae el repositorio completo"):
            iniciar.validar_proyecto(self.raiz)

    def test_firmware_vacio(self):
        (self.raiz / "firmware" / "pot_led.elf").write_bytes(b"")
        with self.assertRaisesRegex(iniciar.ErrorPreparacion, "vacio"):
            iniciar.validar_proyecto(self.raiz)

    def test_toml_invalido(self):
        (self.carpeta / "wokwi.toml").write_text("[wokwi", encoding="utf-8")
        with self.assertRaises(iniciar.ErrorPreparacion):
            iniciar.validar_proyecto(self.raiz)

    def test_toml_faltante_indica_carpeta_correcta(self):
        (self.carpeta / "wokwi.toml").unlink()
        with self.assertRaisesRegex(iniciar.ErrorPreparacion, "wokwi_emulation.*wokwi.toml"):
            iniciar.validar_proyecto(self.raiz)

    def test_ruta_fuera_del_proyecto(self):
        (self.carpeta / "wokwi.toml").write_text(
            '[wokwi]\nversion=1\nfirmware="../../ajeno.hex"\nelf="../firmware/pot_led.elf"',
            encoding="utf-8",
        )
        with self.assertRaisesRegex(iniciar.ErrorPreparacion, "dentro del proyecto"):
            iniciar.validar_proyecto(self.raiz)

    def test_json_no_es_circuito(self):
        (self.carpeta / "diagram.json").write_text("[]", encoding="utf-8")
        with self.assertRaises(iniciar.ErrorPreparacion):
            iniciar.validar_proyecto(self.raiz)

    def test_conexion_a0_incorrecta(self):
        archivo = self.carpeta / "diagram.json"
        circuito = json.loads(archivo.read_text(encoding="utf-8"))
        circuito["connections"][2][1] = "uno:A1"
        archivo.write_text(json.dumps(circuito), encoding="utf-8")
        with self.assertRaisesRegex(iniciar.ErrorPreparacion, "conexion"):
            iniciar.validar_proyecto(self.raiz)

    def test_comprobar_no_instala_ni_abre(self):
        patch("iniciar.shutil.which", return_value="code").start()
        proceso = patch("iniciar.ejecutar_code", return_value="Wokwi.wokwi-vscode\n").start()
        self.assertEqual(self.ejecutar(["--comprobar"]), 0)
        proceso.assert_called_once_with("code", ["--list-extensions"])

    def test_comprobar_informa_extension_faltante(self):
        patch("iniciar.shutil.which", return_value="code").start()
        proceso = patch("iniciar.ejecutar_code", return_value="").start()
        self.assertEqual(self.ejecutar(["--comprobar"]), 1)
        self.assertIn("sin --comprobar", self.salida.getvalue())
        self.assertEqual(proceso.call_count, 1)

    def test_instalar_extension_y_no_abrir(self):
        patch("iniciar.shutil.which", return_value="code").start()
        proceso = patch("iniciar.ejecutar_code", side_effect=["", "Instalada", "wokwi.wokwi-vscode"]).start()
        self.assertEqual(self.ejecutar(["--sin-abrir"]), 0)
        self.assertEqual(proceso.call_args_list[1].args[1], ["--install-extension", "wokwi.wokwi-vscode"])
        self.assertEqual(proceso.call_count, 3)

    def test_extension_instalada_abre_sin_reinstalar(self):
        patch("iniciar.shutil.which", return_value="code").start()
        proceso = patch("iniciar.ejecutar_code", side_effect=["wokwi.wokwi-vscode", ""]).start()
        self.assertEqual(self.ejecutar(), 0)
        proceso.assert_called_with("code", ["--new-window", str(self.carpeta)])
        self.assertEqual(proceso.call_count, 2)

    def test_vscode_ausente(self):
        patch("iniciar.shutil.which", return_value=None).start()
        self.assertEqual(self.ejecutar(), 1)
        self.assertIn("No se encuentra el comando code", self.salida.getvalue())

    def test_cli_rechaza_operacion(self):
        resultado = subprocess.CompletedProcess([], 1, "", "Sin conexion")
        with patch("iniciar.subprocess.run", return_value=resultado):
            with self.assertRaisesRegex(iniciar.ErrorPreparacion, "Sin conexion"):
                iniciar.ejecutar_code("code", ["--list-extensions"])

    def test_cli_timeout(self):
        with patch("iniciar.subprocess.run", side_effect=subprocess.TimeoutExpired("code", 60)):
            with self.assertRaisesRegex(iniciar.ErrorPreparacion, "tardo demasiado"):
                iniciar.ejecutar_code("code", ["--list-extensions"])

    def test_cli_preserva_argumentos_sin_shell(self):
        resultado = subprocess.CompletedProcess([], 0, "", "")
        with patch("iniciar.subprocess.run", return_value=resultado) as proceso:
            iniciar.ejecutar_code("/opt/VS Code/bin/code", ["--new-window", str(self.carpeta)])
            self.assertEqual(proceso.call_args.args[0][-1], str(self.carpeta))
            self.assertFalse(proceso.call_args.kwargs.get("shell", False))


if __name__ == "__main__":
    unittest.main()