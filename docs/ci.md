# Integracion continua del firmware

El [workflow](../.github/workflows/mbd-ci-pipeline.yml) verifica el proyecto en
Linux, sin MATLAB, VS Code, Wokwi ni placa conectada. Se ejecuta con `push` y
`pull_request` hacia `main`, y manualmente desde **Actions > Model-Based CI >
Run workflow** una vez que el workflow este en la rama predeterminada.
No requiere secretos. No carga placas ni modifica los binarios de la demo.

## Que comprueba

1. Pruebas Python y estructura del circuito y firmware incluidos.
2. Coincidencia del modelo, fuentes y dependencias con el manifiesto.
3. Compilacion de todas las fuentes de la aplicacion y del nucleo Arduino;
   ningun objeto, biblioteca o ejecutable previo se reutiliza como entrada.
4. Fallos intencionales de compilacion y enlace en copias temporales.
5. Analisis con `cppcheck` y los tamaños reales de los tipos AVR.
6. Limites de 32256 bytes de flash (reservando el bootloader del Uno) y 2048
   bytes de RAM estatica. Esto no mide el maximo de pila ni memoria dinamica.

El compilador AVR `7.3.0-atmel3.6.1-arduino7` y Arduino AVR `1.8.3` se descargan
desde Arduino y se verifican mediante SHA-256. `cppcheck` procede del gestor de
paquetes del runner Ubuntu 24.04; su version se registra en el informe XML.

## Cuando cambia el modelo

Guardar y cerrar el modelo. Desde MATLAB, con el repositorio como carpeta actual:

```matlab
addpath(fullfile(pwd, 'scripts'));
exportar_ci();
```

El [exportador](../scripts/exportar_ci.m) compila en una carpeta temporal,
comprueba las versiones y usa `packNGo` para recoger las fuentes necesarias.
Reemplaza las fuentes exportadas, pero no el modelo ni `firmware/pot_led.*`.
El ZIP de inspeccion queda en `build/ci-export/`, ignorado por Git: no publicarlo.

Revisar y versionar juntos el modelo, las [fuentes](../generated_code/NOTICE.md)
y su [manifiesto](../generated_code/manifest.json). No editar el C generado a
mano ni cambiar el hash para ocultar una falta de regeneracion. El manifiesto
es un control de consistencia, no una prueba criptografica de autoria: la revision
de cambios sigue siendo necesaria.

## Repetir localmente

Requiere Linux x86-64, Python 3.11 actualizado (o posterior), Internet para la
primera descarga y `cppcheck` instalado para el ultimo comando.

```bash
python3 -m unittest discover -s tests -v
python3 scripts/ci_firmware.py verificar
python3 scripts/ci_firmware.py compilar
MINITALLER_TEST_AVR=1 python3 -m unittest discover -s tests -p test_ci_firmware.py -v
python3 scripts/ci_firmware.py analizar
```

`--cppcheck /ruta/cppcheck` permite elegir el analizador. Las herramientas se
descargan bajo `build/ci-tools`; no se instalan globalmente. No ejecutar estos
scripts con `sudo`. No se necesitan las rutas locales de MATLAB para compilar.

## Informes y artefactos

`informes-<commit>` contiene `resumen.json`, la base de comandos de compilacion
y `cppcheck.xml`. Se intenta conservarlos incluso cuando falla una comprobacion.
El resumen registra hashes, versiones y ocupacion de memoria.

`firmware-uno-<commit>` solo se publica si pasan los controles. Contiene HEX, ELF
y un ZIP de fuentes correspondientes para reconstruccion, incluido el nucleo
Arduino con sus avisos. Los artefactos se conservan 14 dias. No son una release
ni reemplazan automaticamente el firmware incluido en el repositorio.

El analisis bloquea errores en cualquier fuente y avisos de tipo warning,
performance o portability en el codigo generado del modelo. Los avisos de las
dependencias se conservan y se muestran, pero no bloquean por si solos.
`missingIncludeSystem` se excluye del analisis: el compilador real ya verifica
los includes; cppcheck no usa necesariamente todos los built-ins de avr-gcc.

## Resultado y limites conocidos

En la comprobacion local del 15 de octubre de 2026, el build independiente ocupa
4780 bytes de flash y 287 bytes de RAM estatica. Una segunda compilacion en otra
ruta reproduce los mismos hashes. Sin embargo, el HEX difiere del generado por
MATLAB: `matches_matlab_hex` es `false`. No se afirma equivalencia funcional a
partir de una compilacion correcta o de cifras de memoria iguales.

La demo fisica y el PWM comprobados por el expositor corresponden al firmware
anterior, que se conserva intacto. Antes de usar el HEX de CI como sustituto,
repetir las pruebas virtuales, fisicas y de PWM con ese artefacto exacto.

El compilador muestra advertencias del `ert_main.c` original sobre `#endif;`
y declaraciones implicitas. No se parchearon ni ocultaron. Cppcheck 2.19 registra
tres avisos en dependencias: `uninitMemberVar` en `Stream.h` de Arduino y dos
`constStatement` en `io_wrappers.cpp` de MathWorks. No son diagnosticos del modelo.

Las fuentes conservan sus licencias y avisos; ver [NOTICE](../generated_code/NOTICE.md).
La habilitacion efectiva de Actions, cuotas del repositorio y primera ejecucion
remota se comprueban despues de subir los cambios. No se ha hecho commit ni push
automaticamente ni se ha ejecutado este workflow en GitHub durante la preparacion.