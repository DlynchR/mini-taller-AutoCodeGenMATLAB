# Bitacora de ejecucion

Este documento registra pasos realmente ejecutados, resultados y pendientes.
No sustituye al tutorial final: las instrucciones de generacion y emulacion se
completaran cuando hayan sido probadas. No registrar claves, tokens ni numeros
de licencia en este repositorio.

## Alcance confirmado

- Tutorial: Arduino Uno R3 emulado en Wokwi para VS Code.
- Ejercicio: potenciometro A0 y control del brillo de un LED mediante PWM D3.
- Demostracion posterior: Arduino Uno R3 fisico del expositor.
- MATLAB y GitHub Actions preparados antes de clase, no construidos por los
  participantes durante el tutorial.
- Primer hito: ejecutar firmware realmente generado por Simulink en Wokwi.

## 2026-10-12: diagnostico inicial

### Entorno observado

| Comprobacion | Resultado |
| --- | --- |
| Sistema anfitrion | Linux, arquitectura x86_64 |
| MATLAB | R2026b, version 26.2.0.3386108 |
| Instalacion local | `/home/dlynchr/Matlab` |
| Productos registrados | MATLAB y MATLAB Copilot |
| Simulink / Simulink Coder / Embedded Coder | No instalados segun `ver` |
| Complementos registrados | MATLAB Copilot 26.2, habilitado |
| MATLAB en `PATH` del terminal | No encontrado; se usa ruta absoluta |
| Espacio libre en la particion de instalacion | 167 GB al comprobar |

La existencia de carpetas de productos no demuestra que esten instalados.
El usuario reporto intentos anteriores de instalacion fallidos. La comprobacion
de productos tampoco demuestra disponibilidad de licencias ni compilacion.

### Repetir el diagnostico

Desde la raiz del repositorio, en un terminal Linux, ajustar la ruta de MATLAB
si la instalacion se encuentra en otro directorio:

```bash
MATLAB_ROOT="/home/dlynchr/Matlab"
"$MATLAB_ROOT/bin/matlab" -batch "addpath(fullfile(pwd, 'scripts')); estado = verificar_entorno(); assert(estado.simulinkDisponible, 'minitaller:SimulinkNoDisponible', 'Simulink no esta instalado o no se encuentra en el path de MATLAB.');"
```

El [diagnostico](../scripts/verificar_entorno.m) imprime productos y complementos
sin mostrar el numero de licencia. Puede ejecutarse tambien desde la Command
Window de MATLAB, con el repositorio como carpeta actual:

```matlab
addpath(fullfile(pwd, 'scripts'));
estado = verificar_entorno();
```

Resultado inicial comprobado: `simulinkDisponible = 0` y fallo del `assert` por
ausencia de Simulink. Es un bloqueo del entorno, no una prueba de firmware.

## Preparacion de la instalacion

Se eligio MATLAB Package Manager (`mpm`), publicado por MathWorks, porque permite
anadir productos y dependencias y omite productos ya instalados. No requiere
cambiar el `PATH` global. El usuario confirmo acceso a productos y cerro MATLAB
antes de modificar su instalacion. No cerrar procesos con trabajo sin guardar.

Estos comandos se ejecutaron para descargar la herramienta y consultar su ayuda:

```bash
command -v curl
command -v unzip
MPM_DIR=$(mktemp -d /tmp/minitaller-mpm.XXXXXX)
curl --fail --location --silent --show-error \
  https://www.mathworks.com/mpm/glnxa64/mpm --output "$MPM_DIR/mpm"
chmod u+x "$MPM_DIR/mpm"
"$MPM_DIR/mpm" --version
"$MPM_DIR/mpm" install --help
```

Version observada: MATLAB Package Manager 2026.7.1, plataforma `glnxa64`.
La variable `MPM_DIR` solo existe en ese terminal. La descarga queda fuera del
repositorio; no se versionan instaladores ni credenciales.

### Instalacion de productos completada

Con MATLAB cerrado, se ejecuto en el mismo terminal:

```bash
"$MPM_DIR/mpm" install --release=R2026b \
  --destination=/home/dlynchr/Matlab \
  --products=Simulink Simulink_Coder Embedded_Coder
```

El instalador anadio MATLAB Coder como dependencia y termino con
`Installation complete`. No se necesito `sudo`. Los productos Coder se incluyen
por los objetivos de exportacion de codigo y trazabilidad del taller; no se
afirma que sean obligatorios para cualquier uso del soporte Arduino.

### Verificacion posterior: aprobada

Se repitio el diagnostico y se verifico la creacion, carga y cierre de un modelo
en memoria, sin guardar archivos:

```bash
"/home/dlynchr/Matlab/bin/matlab" -batch "addpath(fullfile(pwd, 'scripts')); estado = verificar_entorno(); assert(estado.simulinkDisponible && estado.simulinkCoderInstalado && estado.embeddedCoderInstalado, 'minitaller:ProductosNoDisponibles', 'Faltan productos del flujo de generacion.'); modeloPrueba = 'verificacion_instalacion_minitaller'; new_system(modeloPrueba); load_system(modeloPrueba); close_system(modeloPrueba, 0); fprintf('PRUEBA OK: Simulink crea, carga y cierra un modelo en memoria.\n');"
```

Resultado: Simulink, Simulink Coder, MATLAB Coder y Embedded Coder aparecen con
version 26.2. La prueba del modelo finalizo correctamente. Esto verifica el uso
basico de Simulink, no todavia una compilacion ni el uso efectivo de Coder.

## Soporte Arduino instalado y comprobado

Se ejecuto, con MATLAB cerrado:

```bash
"$MPM_DIR/mpm" install --release=R2026b \
  --destination=/home/dlynchr/Matlab \
  --products=Simulink_Support_Package_for_Arduino_Hardware
```

El instalador acepto el identificador, no solicito un indicador adicional de
aceptacion de licencias de terceros y termino correctamente. Destino observado:
`/home/dlynchr/Documents/MATLAB/SupportPackages/R2026b`.

Verificaciones ejecutadas en MATLAB:

```matlab
verificar_entorno();
which('arduinolib')
load_system('arduinolib');
find_system('arduinolib', 'SearchDepth', 2, ...
    'RegExp', 'on', 'Name', 'Analog Input|PWM')
close_system('arduinolib', 0);
```

Resultado: soporte version 26.2.3, habilitado. La biblioteca contiene
`arduinolib/Analog Input` y `arduinolib/PWM`. El parametro de pin de ambos es
`pinNumber`; el de muestreo de Analog Input es `SampleTime`.

## Modelo minimo reproducible

Se creo [preparar_modelo.m](../scripts/preparar_modelo.m) y se ejecuto desde la
raiz del repositorio:

```bash
"/home/dlynchr/Matlab/bin/matlab" -batch "addpath(fullfile(pwd, 'scripts')); preparar_modelo();"
```

El resultado es [pot_led.slx](../model/pot_led.slx). El script no sobrescribe un
modelo existente ni cierra un modelo del mismo nombre que el usuario tenga
abierto. Para reconstruir desde cero, usar una copia de trabajo separada sin
el modelo, conservando el original.

| Decision | Configuracion del modelo |
| --- | --- |
| Placa | Arduino Uno, ATmega328P |
| Entrada | Analog Input, pin 0 (A0), `uint16` |
| Muestreo | Fixed-step, FixedStepDiscrete, 0.01 s |
| Escalamiento | Conversion a `single`, ganancia `single(255/1023)` |
| Limites | Saturation entre 0 y 255 |
| Conversion PWM | `uint8`, redondeo Floor, saturacion habilitada |
| Salida | PWM, pin 3, frecuencia predeterminada del soporte |
| Generacion | `ert.tlc`, lenguaje C, reporte habilitado |

Se verificaron mediante `get_param` la placa, los pines y el paso fijo. La
revision del C generado confirma la lectura A0, el escalamiento, la saturacion,
la conversion y la llamada al controlador PWM de D3. El pin 14 que aparece en
el C corresponde al identificador Arduino de A0; no es una conexion adicional.

## Fallo encontrado: descarga automatica despues del build

Una consulta inicial a `TargetHardwareResources` fallo: ese parametro no existe
en este modelo R2026b. La API `codertarget.data.getData` requiere el objeto
`ConfigSet` en este flujo; pasar el nombre del modelo produjo un error.

La primera llamada a `slbuild` llego al hook de descarga y fallo con
`No Arduino Uno board detected`. No se conecto una placa para sortear el error.
La documentacion generica de `slbuild` no era suficiente para describir la
accion posterior que agrega el target Arduino.

Inspeccion que identifico la causa:

```matlab
configuracion = getActiveConfigSet('pot_led');
datosTarget = codertarget.data.getData(configuracion);
datosTarget.Runtime
```

Valor observado: `BuildAction = 'Build, load and run'`. Se corrigio a `Build`:

```matlab
codertarget.data.setParameterValue(configuracion, ...
    'Runtime.BuildAction', 'Build');
```

La correccion se guardo en el modelo y se incorporo al generador. El
[script de compilacion](../scripts/compilar_modelo.m) tambien la establece y
comprueba antes de llamar a `slbuild`. Estas APIs se probaron en R2026b; no se
afirma compatibilidad con versiones anteriores sin validacion.

## Compilacion sin placa: aprobada

Comando reproducible desde la raiz del repositorio:

```bash
"/home/dlynchr/Matlab/bin/matlab" -batch "addpath(fullfile(pwd, 'scripts')); resultado = compilar_modelo(); disp(resultado);"
```

Resultado: `Successful completion of build procedure for: pot_led`, con codigo
generado y compilado, sin error de descarga. El script encontro un HEX y un ELF
y los copio a la carpeta `firmware`.

Datos observados del toolchain y del build:

- Compiladores AVR del soporte: `avr-gcc` y `avr-g++`, distribucion
  `7.3.0-atmel3.6.1-arduino7`.
- Arduino AVR core 1.8.3, target `atmega328p`, `F_CPU=16000000L`.
- Se enlazan fuentes C del modelo, fuentes C++ del soporte, runtime, scheduler
  y core Arduino. Un comando sobre solo los archivos C del modelo no reproduce
  este enlace; la receta independiente para CI sigue pendiente.
- El codigo generado contiene referencias a los bloques del modelo y una
  cabecera Academic License. No se eliminaron avisos ni se redistribuyeron
  dependencias bajo una licencia distinta.

Medicion ejecutada con `avr-size -C --mcu=atmega328p` del toolchain instalado:

| Recurso | Uso estatico observado |
| --- | --- |
| Programa | 4780 bytes, 14.6% de los 32768 bytes del microcontrolador |
| Datos | 287 bytes, 14.0% de los 2048 bytes de SRAM |

La cifra de programa no descuenta espacio reservado por un bootloader. La de
datos no mide el maximo de pila ni memoria dinamica durante ejecucion.

Identificadores SHA-256 de los artefactos de esta compilacion:

| Archivo | SHA-256 |
| --- | --- |
| [pot_led.hex](../firmware/pot_led.hex) | `0c0d4ba66ff4b5f1f5141a7bf72c038129fe0c6a407243ee7cd552d58457db30` |
| [pot_led.elf](../firmware/pot_led.elf) | `ac52805b1029e8fe24f5b704a7955eae8e31770bee213ce0eab1d802c2881357` |

Una recompilacion puede modificar estos hashes; compararlos contra la version
de firmware que se vaya a entregar, no contra una supuesta identidad universal.

## Preparacion de Wokwi

Se crearon [diagram.json](../wokwi_emulation/diagram.json) y
[wokwi.toml](../wokwi_emulation/wokwi.toml). El circuito une 5V/GND al
potenciometro y su salida a A0; D3 alimenta una resistencia virtual de 330 ohmios
y el anodo del LED, con su catodo a GND. La resistencia real se verificara antes
de la demo fisica.

La configuracion usa rutas relativas al TOML: `../firmware/pot_led.hex` y
`../firmware/pot_led.elf`. Se comprobaron con los parsers estandar `json` y
`tomllib` de Python las sintaxis, las rutas de ambos archivos, los IDs unicos y
las conexiones principales. Es validacion estatica, no emulacion.

La extension no estaba instalada. Se ejecuto:

```bash
code --install-extension wokwi.wokwi-vscode
```

Resultado: extension 4.0.0 instalada correctamente. El expositor confirmo que
el simulador inicio y que el LED responde al potenciometro. No se consultaron
claves ni se registro el tipo, costo o vigencia de su licencia.

En una segunda comprobacion guiada, el expositor confirmo cada resultado:

| Caso | Observacion manual reportada |
| --- | --- |
| Minimo | El LED se apaga |
| Posicion intermedia | El brillo es intermedio |
| Maximo | El LED alcanza su mayor brillo |
| Regreso al minimo | El LED vuelve a apagarse |
| Detener y reiniciar | El comportamiento se repite |

Primer hito virtual completado el 12 de octubre de 2026. Evidencia de esta prueba:
confirmacion explicita del expositor durante la ejecucion guiada. No se capturo
una forma de onda ni se ejecuto una prueba automatizada en Wokwi. La validacion
no demuestra todavia una frecuencia PWM, un periodo de ejecucion medido ni
equivalencia con la placa fisica.

Las instrucciones para repetirla y su tabla de comprobacion estan en el
[README](../README.md#primera-ejecucion-virtual).

## Pendientes al cierre de esta etapa

- Repetir el tutorial con otro participante y comprobar su acceso a Wokwi.
- Medir PWM y decidir la instrumentacion antes de agregarla; el firmware actual
  no envia datos por UART.
- Preparar exportacion reproducible y legalmente distribuible para CI.
- Montar y probar la demo fisica; generar los materiales academicos con evidencia
  del trabajo en NotebookLM. No se han declarado estos entregables completos.

## Guia provisional para la primera defensa

Se preparo `guia-provisional.md` (ya retirada) para explicar el avance
desde la preparacion de MATLAB hasta la apertura del modelo y su ejecucion
virtual. Distingue la instalacion inicial, ya existente, de las instalaciones
realmente ejecutadas durante la sesion. Incluye mapa de archivos, fundamento
de los bloques, pasos reproducibles, evidencias, limitaciones y guion de defensa.

Se agregaron comentarios a las tres funciones de la carpeta `scripts`, sin
cambiar sus instrucciones ejecutables. Se comprobo esa condicion comparando
los cambios con el indice de Git. No se regeneraron el modelo ni el firmware
para esta mejora documental; las pruebas manuales registradas siguen referidas
a los artefactos de la primera etapa.

Se exporto una [imagen del modelo real](figuras/modelo-pot-led.png) mediante
`print('-spot_led', '-dpng', '-r150', rutaImagen)` en una sesion MATLAB por lotes,
tras cargar el modelo guardado. La imagen se incorporo a la guia sin guardar
cambios en el modelo ni recompilar el firmware.

Se anadio `exportar_guia.py` (ya retirado), con comentarios, para
producir el PDF provisional (ya retirado) desde el Markdown. Usa
Python-Markdown 3.7 en `build/pdf-venv` y Chrome en modo headless con un perfil
temporal. La guia documenta como repetir la exportacion. Se comprobaron enlaces
locales, extraccion de texto del PDF y paginas representativas renderizadas.
Esta revision documental no sustituye ni repite las pruebas del firmware.

## Referencia consultada

- MathWorks, [Get MATLAB Package Manager](https://www.mathworks.com/help/install/ug/get-mpm-os-command-line.html),
  consultada durante la preparacion. Instalar productos no concede una licencia
  para utilizarlos; el acceso se verifica en el entorno del expositor.
- MathWorks, [Install Support Packages Noninteractively](https://www.mathworks.com/help/install/ug/matlab-package-manager.html).
- MathWorks, [Install Support for Arduino Hardware](https://www.mathworks.com/help/simulink/supportpkg/arduino_ug/install-support-for-arduino-hardware.html).
- MathWorks, [slbuild](https://www.mathworks.com/help/simulink/slref/slbuild.html).
- MathWorks, [Run Model on Arduino Hardware](https://www.mathworks.com/help/simulink/supportpkg/arduino_ug/run-model-on-arduino-hardware.html).
- Wokwi, [Configuring Your Project](https://docs.wokwi.com/vscode/project-config).