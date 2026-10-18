"""Pruebas de instalacion sin descargar mpm ni cambiar MATLAB."""

from contextlib import redirect_stdout
from io import BytesIO, StringIO
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
    @patch.object(instalar, "ejecutar_mpm")
    @patch.object(instalar.subprocess, "run")
    @patch.object(instalar.urllib.request, "urlopen")
    def test_instala_y_verifica_solo_tras_confirmacion(self, descarga, ejecutar, ejecutar_mpm, plataforma):
        descarga.return_value.__enter__.return_value = BytesIO(b"mpm de prueba")
        self.assertEqual(instalar.preparar(str(self.raiz), confirmar=lambda _: "si"), 0)
        ejecutar_mpm.assert_called_once()
        ejecutar.assert_called_once()
        instalacion = ejecutar_mpm.call_args.args[0]
        self.assertEqual(instalacion[1:4], ["install", "--release=R2026b", f"--destination={self.raiz}"])
        self.assertEqual(instalacion[4:], [f"--products={instalar.PRODUCTOS[0]}", *instalar.PRODUCTOS[1:]])
        self.assertEqual(ejecutar.call_args.args[0][:2], [str(self.matlab), "-batch"])
        self.assertIn("Faltan bloques Arduino", ejecutar.call_args.args[0][2])
        self.assertTrue(ejecutar.call_args.kwargs["check"])

    @patch.object(instalar, "plataforma_mpm", return_value=("glnxa64", "mpm"))
    @patch.object(instalar, "ejecutar_mpm", side_effect=subprocess.CalledProcessError(255, "mpm"))
    @patch.object(instalar.subprocess, "run")
    @patch.object(instalar.urllib.request, "urlopen")
    def test_fallo_mpm_no_verifica_matlab(self, descarga, ejecutar, ejecutar_mpm, plataforma):
        descarga.return_value.__enter__.return_value = BytesIO(b"mpm de prueba")
        with self.assertRaises(subprocess.CalledProcessError):
            instalar.preparar(str(self.raiz), confirmar=lambda _: "si")
        ejecutar_mpm.assert_called_once()
        ejecutar.assert_not_called()

    @patch.object(instalar, "plataforma_mpm", return_value=("glnxa64", "mpm"))
    @patch.object(instalar.subprocess, "Popen")
    @patch.object(instalar.subprocess, "run")
    @patch.object(instalar.urllib.request, "urlopen")
    def test_productos_instalados_se_verifican_antes_de_anunciar_listo(
        self, descarga, verificar, abrir_proceso, plataforma
    ):
        proceso = abrir_proceso.return_value.__enter__.return_value
        proceso.wait.return_value = 255
        for fallo_verificacion in (False, True):
            with self.subTest(fallo_verificacion=fallo_verificacion):
                descarga.return_value.__enter__.return_value = BytesIO(b"mpm de prueba")
                proceso.stdout = StringIO("Starting install\nAll specified products are already installed.\n")
                verificar.reset_mock()
                verificar.side_effect = (subprocess.CalledProcessError(1, "matlab")
                                         if fallo_verificacion else None)
                salida = StringIO()
                with redirect_stdout(salida):
                    if fallo_verificacion:
                        with self.assertRaises(subprocess.CalledProcessError):
                            instalar.preparar(str(self.raiz), confirmar=lambda _: "si")
                    else:
                        self.assertEqual(instalar.preparar(str(self.raiz), confirmar=lambda _: "si"), 0)
                verificar.assert_called_once()
                self.assertTrue(verificar.call_args.kwargs["check"])
                self.assertIn("Los productos ya estan instalados", salida.getvalue())
                self.assertEqual("LISTO: dependencias MATLAB verificadas" in salida.getvalue(),
                                 not fallo_verificacion)

    @patch.object(instalar.urllib.request, "urlopen")
    def test_cancelacion_no_descarga(self, descarga):
        self.assertEqual(instalar.preparar(str(self.raiz), confirmar=lambda _: "no"), 0)
        descarga.assert_not_called()


class PruebasResultadoMPM(unittest.TestCase):
    def test_clasifica_solo_el_caso_conocido_y_conserva_la_salida(self):
        instalado = "All specified products are already installed."
        casos = ((0, "Installation complete.\n", False),
                 (255, f"Starting install\n  {instalado}\r\n", False),
                 (255, "No permission to write destination.\n", True),
                 (255, "", True),
                 (255, f"Error: {instalado}\n", True),
                 (1, instalado + "\n", True))
        for codigo, texto, falla in casos:
            with self.subTest(codigo=codigo, texto=texto), \
                 patch.object(instalar.subprocess, "Popen") as abrir_proceso:
                proceso = abrir_proceso.return_value.__enter__.return_value
                proceso.stdout = StringIO(texto)
                proceso.wait.return_value = codigo
                salida = StringIO()
                comando = ["/ruta con espacios/mpm", "install"]
                with redirect_stdout(salida):
                    if falla:
                        with self.assertRaises(subprocess.CalledProcessError) as error:
                            instalar.ejecutar_mpm(comando)
                        self.assertEqual(error.exception.returncode, codigo)
                        self.assertEqual(error.exception.output, texto)
                    else:
                        instalar.ejecutar_mpm(comando)
                self.assertTrue(salida.getvalue().startswith(texto))
                abrir_proceso.assert_called_once_with(
                    comando, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding="utf-8", errors="replace")


if __name__ == "__main__":
    unittest.main()