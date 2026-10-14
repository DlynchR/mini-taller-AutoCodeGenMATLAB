"""Pruebas de instalacion sin descargar mpm ni cambiar MATLAB."""

from io import BytesIO
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from scripts import instalar_dependencias as instalar


class PruebasInstalacion(unittest.TestCase):
    def setUp(self):
        temporal = tempfile.TemporaryDirectory(prefix="matlab de prueba ")
        self.addCleanup(temporal.cleanup)
        self.raiz = Path(temporal.name) / "MATLAB con espacios"
        (self.raiz / "bin").mkdir(parents=True)
        self.matlab = self.raiz / "bin" / "matlab"
        self.matlab.touch()
        (self.raiz / "VersionInfo.xml").write_text(
            "<MathWorks_version_info><release>R2026b</release></MathWorks_version_info>"
        )

    def test_lee_release_de_la_instalacion_elegida(self):
        self.assertEqual(instalar.instalacion(str(self.raiz)), (self.raiz, self.matlab, "R2026b"))

    def test_rechaza_ruta_y_release_invalidas(self):
        with self.assertRaisesRegex(ValueError, "No se encontro MATLAB"):
            instalar.instalacion(str(self.raiz / "otra"))
        (self.raiz / "VersionInfo.xml").write_text("<release>invalida</release>")
        with self.assertRaisesRegex(ValueError, "no reconocida"):
            instalar.instalacion(str(self.raiz))

    @patch.object(instalar.platform, "system", return_value="Linux")
    @patch.object(instalar.platform, "machine", return_value="aarch64")
    def test_rechaza_arquitectura_no_soportada(self, arquitectura, sistema):
        with self.assertRaisesRegex(ValueError, "requiere Linux x86-64"):
            instalar.plataforma_mpm()

    @patch.object(instalar, "plataforma_mpm", return_value=("glnxa64", "mpm"))
    @patch.object(instalar.subprocess, "run")
    @patch.object(instalar.urllib.request, "urlopen")
    def test_instala_y_verifica_solo_tras_confirmacion(self, descarga, ejecutar, plataforma):
        descarga.return_value.__enter__.return_value = BytesIO(b"mpm de prueba")
        self.assertEqual(instalar.preparar(str(self.raiz), confirmar=lambda _: "si"), 0)
        self.assertEqual(ejecutar.call_count, 2)
        instalacion = ejecutar.call_args_list[0].args[0]
        self.assertEqual(instalacion[1:4], ["install", "--release=R2026b", f"--destination={self.raiz}"])
        self.assertEqual(instalacion[4:], [f"--products={instalar.PRODUCTOS[0]}", *instalar.PRODUCTOS[1:]])
        self.assertEqual(ejecutar.call_args_list[1].args[0][:2], [str(self.matlab), "-batch"])
        self.assertIn("Faltan bloques Arduino", ejecutar.call_args_list[1].args[0][2])
        self.assertTrue(ejecutar.call_args_list[0].kwargs["check"])

    @patch.object(instalar, "plataforma_mpm", return_value=("glnxa64", "mpm"))
    @patch.object(instalar.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "mpm"))
    @patch.object(instalar.urllib.request, "urlopen")
    def test_fallo_mpm_no_verifica_matlab(self, descarga, ejecutar, plataforma):
        descarga.return_value.__enter__.return_value = BytesIO(b"mpm de prueba")
        with self.assertRaises(subprocess.CalledProcessError):
            instalar.preparar(str(self.raiz), confirmar=lambda _: "si")
        self.assertEqual(ejecutar.call_count, 1)

    @patch.object(instalar.urllib.request, "urlopen")
    def test_cancelacion_no_descarga(self, descarga):
        self.assertEqual(instalar.preparar(str(self.raiz), confirmar=lambda _: "no"), 0)
        descarga.assert_not_called()


if __name__ == "__main__":
    unittest.main()