# Mini-taller: AutoCodeGenMATLAB

## Parte 1: MATLAB Coder Onramp

Como parte del Mini-Taller y en coordinación con el profesor, va a ser necesario completar el curso "MATLAB Coder Onramp" que ofrece la plataforma Matlab Academy. La idea es que el título de este curso sea la evidencia de haber completado el Mini-Taller, además de certificarnos en una herramienta muy útil de Matlab. Este es el enlace del curso:

[MATLAB Coder Onramp](https://matlabacademy.mathworks.com/es/details/matlab-coder-onramp/ormc)

## Parte 2: De Simulink a Wokwi

En este ejercicio un potenciómetro conectado a A0 regula el brillo de un LED
conectado a D3 mediante PWM en un Arduino Uno R3. 

El diagrama de Simulink se
convierte en código C/C++, se compila para el ATmega328P y el archivo resultante
se ejecuta en un circuito virtual. El Uno digitaliza la tensión del potenciómetro
en una lectura entre 0 y 1023 (10 bits); el modelo la convierte en un valor de
control entre 0 y 255 (8 bits) para el bloque PWM. Al mover el potenciómetro
cambiará el brillo del LED. El período de actualización del modelo es 0.01 s. **No hace falta una placa física.**

![Diagrama de los bloques del modelo](docs/figuras/modelo-pot-led.png)

El asistente y la ruta con MATLAB están preparados para **Linux x86-64**.
La ruta rápida también se puede hacer en Windows instalando manualmente la
extensión Wokwi, como se explica [más abajo](#ruta-windows).

## Elige tu ruta

| Ruta | Qué harás | Requisitos |
| --- | --- | --- |
| **[A: Rápida, sin MATLAB](#ruta-rapida)** | Usar el firmware incluido, abrir Wokwi y experimentar; ir directamente a los pasos 4 y 5 | VS Code, Internet y activación Wokwi; Python 3.11+ para el asistente Linux |
| **[B: Completa, con MATLAB](#ruta-matlab)** | Preparar MATLAB, inspeccionar el modelo, generar código y compilar; después simular en Wokwi | Linux x86-64, Python 3.11+, VS Code, Internet, activación Wokwi y MATLAB con acceso a los productos necesarios |

Ambas rutas parten del [repositorio completo](#preparar-el-repositorio) y
terminan con el mismo circuito virtual. **No necesitas MATLAB para la ruta A.**

**Navegación:**

[0. Preparar el Repositorio (Ambas rutas)](#preparar-el-repositorio)

[1. Preparar MATLAB (Solo Ruta B)](#1-preparar-matlab-solo-ruta-b)

[2. Explorar el modelo (Solo Ruta B)](#2-explorar-el-modelo-solo-ruta-b)

[3. Generar firmware (Solo Ruta B)](#3-generar-firmware-solo-ruta-b)

[4. Abrir el circuito (Ambas rutas)](#4-abrir-el-circuito-ambas-rutas)

[5. Verificar (Ambas rutas)](#5-verificar-el-comportamiento-ambas-rutas)

[6. Cargar en hardware real (Opcional)](#6-carga-en-hardware-real-opcional)

[7. Problemas habituales](#problemas-habituales)

## Preparar el repositorio

Clona el repositorio o descárgalo completo con **Code > Download ZIP**, extráelo
y abre esa carpeta en VS Code (**File > Open Folder**). Puedes guardarla en
cualquier ubicación y cambiarle el nombre. Debes ver [iniciar.py](iniciar.py)
junto a las carpetas del proyecto.

Continúa con la **[ruta rápida](#ruta-rapida)** o con la
**[preparación de MATLAB](#ruta-matlab)**.

<a id="ruta-matlab"></a>

## 1. Preparar MATLAB (solo ruta B)

Esta sección es para la ruta completa. Para usar el firmware incluido, salta
a la **[ruta rápida](#ruta-rapida)**.

Instala **MATLAB** con tu cuenta desde la
[descarga oficial de MathWorks](https://www.mathworks.com/downloads/). Guarda
la ubicación que elijas: puede ser cualquier carpeta que permita añadir productos.
No hace falta seleccionar manualmente los add-ons. Necesitas licencia o acceso
válido a los productos; instalarlos no concede una licencia. El recorrido se
comprobó con **R2026b en Linux**; para otra versión, revisa la compatibilidad
del soporte Arduino con tu MATLAB.

Con MATLAB **cerrado** y el [repositorio completo](#preparar-el-repositorio)
abierto en VS Code, abre **Terminal > New Terminal** y ejecuta:

```bash
python3 scripts/instalar_dependencias.py
```

Cuando lo pida, pega la **ruta de la carpeta raíz de MATLAB**, no el ejecutable
ni la carpeta `bin`: por ejemplo, `/home/tu_usuario/Matlab`. En su interior
deben existir `VersionInfo.xml` y `bin/matlab`. El asistente lee la versión de esa
carpeta, muestra su destino y pide escribir `si` antes de cambiar la instalación.
Si hay más de una versión instalada, indica **la que abrirás después**.

El asistente descarga la versión actual de **MATLAB Package Manager** desde
MathWorks a una carpeta temporal, añade **Simulink**, **Simulink Coder**,
**Embedded Coder** y **Simulink Support Package for Arduino Hardware** a esa
instalación; MathWorks resuelve dependencias y omite productos ya instalados.
Si todos los productos ya están instalados, el asistente continúa con su
verificación; ese caso no se trata como un error de instalación.
La descarga automática requiere Linux x86-64; en otra arquitectura, el asistente
se detiene antes de instalar. Puedes indicar
la ubicación directamente con `--ruta "ruta de la carpeta de MATLAB"`.
La descarga necesita Internet y puede tardar. No se almacenan credenciales en
el repositorio ni se configura un puerto USB. Al final abre MATLAB en modo
batch, comprueba los productos y carga la biblioteca Arduino. Debe mostrar
`LISTO: dependencias MATLAB verificadas`. Si muestra `NO LISTO`, lee el error,
comprueba conexión, acceso a los productos y permisos de escritura; luego
repite el comando. **No ejecutes el script con `sudo`**: si tu instalación es
del sistema y no puedes escribir en ella, solicita acceso al administrador.
El script automatiza la instalación, pero no puede resolver licencias o restricciones de
una instalación administrada por tu institución.

El script verifica la instalación de paquetes; también puedes comprobarla tú
mismo. Abre **esa misma instalación** de MATLAB y configura como **Current
Folder** la carpeta completa de este repositorio.

![MATLAB abierto con la carpeta del repositorio como Current Folder](docs/figuras/matlab-repositorio-abierto.png)

*Figura 1. MATLAB listo para ejecutar los comandos del tutorial desde la raíz del repositorio.*

En su Command Window, escribe:

```matlab
addpath(fullfile(pwd, 'scripts'));
estado = verificar_entorno();
```

La función muestra los productos y complementos instalados, sin imprimir el
número de licencia.

![Salida de verificar_entorno en MATLAB, incluidos Simulink y el soporte Arduino](docs/figuras/verificacion-entorno-matlab.png)

*Figura 2. Verificación del entorno: Simulink y el soporte Arduino aparecen instalados.*

Debe indicar Simulink localizable y mostrar el soporte
Arduino entre los complementos. Para ver los bloques instalados:

```matlab
load_system('arduinolib');
find_system('arduinolib', 'SearchDepth', 2, 'RegExp', 'on', 'Name', 'Analog Input|PWM')
close_system('arduinolib', 0);
```

El resultado debe incluir `arduinolib/Analog Input` y `arduinolib/PWM`.

**Siguiente:** [Explorar el modelo](#2-explorar-el-modelo-solo-ruta-b) ·
[Volver a elegir ruta](#elige-tu-ruta)

## 2. Explorar el modelo (solo ruta B)

Sin cambiar de **Current Folder**, abre el archivo incluido:

```matlab
open_system(fullfile(pwd, 'model', 'pot_led.slx'));
set_param('pot_led', 'ZoomFactor', 'FitSystem');
```

Los bloques representan el **algoritmo**, no los cables físicos:

| Bloque | Qué hace |
| --- | --- |
| `Lectura_A0` | Lee el potenciómetro por A0 cada 0.01 s; entrega `uint16` |
| `ADC_a_single` | Convierte la lectura a precisión simple para escalarla |
| `Escala_PWM` | Multiplica por `255/1023` para adaptar el rango del ADC al PWM |
| `Limites_PWM` | Limita el valor entre 0 y 255 |
| `PWM_a_uint8` | Convierte al tipo de 8 bits; descarta la fracción para valores no negativos |
| `LED_D3` | Entrega el mando PWM al pin D3 |

Por ejemplo: entrada 0 produce 0; 512 produce aproximadamente 127 tras la
conversión; 1023 produce 255. El brillo percibido no tiene por qué ser lineal.
Abre los bloques con doble clic para inspeccionar sus parámetros. En **Model
Settings** puedes ver `Arduino Uno`, solver discreto y paso fijo `0.01`.

Si necesitas **reconstruir** el diagrama en lugar de usar el incluido, ejecuta
`preparar_modelo()` en una **copia nueva del repositorio donde aún no exista**
el archivo del modelo. La función protege el archivo existente y se detiene en
vez de sobrescribirlo. Para este tutorial no hace falta reconstruirlo.

**Siguiente:** [Generar firmware](#3-generar-firmware-solo-ruta-b) ·
[Volver a elegir ruta](#elige-tu-ruta)

## 3. Generar firmware (solo ruta B)

Guarda y cierra el modelo ejecutando lo siguiente (el script carga la versión guardada):

```matlab
save_system('pot_led');
close_system('pot_led');
resultado = compilar_modelo();
```

Si cerraste MATLAB después del paso 1, vuelve a abrir la carpeta del repositorio
y ejecuta primero `addpath(fullfile(pwd, 'scripts'));`.

**Qué ocurre:** Simulink genera C del modelo; el toolchain AVR compila ese C,
el soporte de Arduino y las dependencias C++, y enlaza un programa para el Uno.
El script configura explícitamente `Runtime.BuildAction = 'Build'`: construir
**sin cargar a una placa**. No uses **Build, Deploy & Start** en esta ruta virtual.

El código, caché y reporte locales quedan en `build/` (excluida de Git).
Después de un build correcto, el script copia `pot_led.hex` y `pot_led.elf` a
`firmware/`. El HEX contiene la imagen para ejecutar; el ELF conserva símbolos
útiles para analizar el programa. Un error de compilación no equivale a firmware
nuevo, si falla, conserva el FW ya funcional del repositorio.

Puedes inspeccionar `build/pot_led_ert_rtw/pot_led.c` si quieres ver cómo los
bloques se convierten en lectura de A0, escalamiento, límites y salida PWM.
Ese C no basta por sí solo para enlazar el programa: también participan los
controladores y el core Arduino. El código generado y los componentes de
MathWorks conservan sus propias condiciones de licencia.

**Siguiente:** [Abrir el circuito](#ruta-rapida) ·
[Volver a elegir ruta](#elige-tu-ruta)

<a id="ruta-rapida"></a>
<a id="primera-ejecucion-virtual"></a>

## 4. Abrir el circuito (ambas rutas)

**Ruta A:** empieza aquí con el firmware incluido; no necesitas ejecutar los
pasos de MATLAB. **Ruta B:** continúa aquí después de compilar.

Abre la carpeta **completa del repositorio** en VS Code. Si aún no la tienes,
sigue la [preparación del repositorio](#preparar-el-repositorio).
Wokwi requiere una activación válida; no se presupone una licencia gratuita
permanente.

### Linux: usar el asistente

Necesitas **Python 3.11 o posterior** e Internet. En VS Code, abre
**Terminal > New Terminal** desde la raíz del repositorio:

```bash
python3 iniciar.py
```

El asistente comprueba que los archivos del circuito y firmware existen,
valida el cableado previsto e instala la extensión Wokwi si no está instalada.
Abre **otra ventana** de VS Code directamente en la carpeta del circuito; no
modifica el modelo ni compila MATLAB. Conserva la ventana original para repetir
el comando. Espera el mensaje `PREPARADO`: esto no significa que la simulación
ya esté ejecutándose. Si indica `NO LISTO`, lee la causa y corrígela antes de
seguir.

Para comprobar sin instalar ni abrir ventanas, agrega `--comprobar`. Para
preparar sin abrir la ventana, agrega `--sin-abrir`.
Continúa con [activar e iniciar Wokwi](#activar-e-iniciar-wokwi) en la nueva ventana.

<a id="ruta-windows"></a>

### Windows: preparación manual

La ruta rápida también funciona en Windows con el firmware incluido. Instala
manualmente **Wokwi Simulator** desde **Extensions** en VS Code. Luego usa
**File > Open Folder** para abrir la carpeta `wokwi_emulation` del repositorio
completo. Debes ver `wokwi.toml` y `diagram.json`; no muevas ni separes la carpeta
`firmware`, porque la configuración usa rutas relativas hacia ella.

No ejecutes el asistente Linux ni los pasos de MATLAB de esta guía. Continúa
con la activación y la simulación en esa ventana.

### Activar e iniciar Wokwi

En la ventana dedicada al circuito:

1. Si VS Code pide confianza, confirma solo si confías en el repositorio.
2. Si la extensión solicita activación, usa `F1` > **Wokwi: Request a New License**
   y completa el proceso en el navegador. No introduzcas claves en el repositorio.
   Si solicita pago, consulta las condiciones del servicio antes de continuar.
   Si ya tienes activación, omite este paso.
3. Pulsa `F1` > **Wokwi: Start Simulator**.

La configuración ya está en la raíz de esa ventana: no tienes que seleccionar
rutas. Si los comandos no aparecen después de instalar la extensión, usa
`F1` > **Developer: Reload Window** tras guardar tu trabajo.

**Siguiente:** [Verificar el comportamiento](#5-verificar-el-comportamiento-ambas-rutas) ·
[Volver a elegir ruta](#elige-tu-ruta)

## 5. Verificar el comportamiento (ambas rutas)

Debes ver Uno, potenciómetro, resistencia y LED. Gira el potenciómetro y anota:

| Acción | Qué deberías observar |
| --- | --- |
| Potenciómetro al mínimo | LED apagado |
| Posición intermedia | LED con brillo intermedio |
| Potenciómetro al máximo | LED con el mayor brillo |
| Regresar al mínimo | LED vuelve a apagarse |
| Detener y reiniciar Wokwi | Se repite el comportamiento |

Se trata de observaciones visuales, no de una medida de frecuencia PWM.
El firmware incluido no envía mensajes serie; un Monitor Serie vacío es normal.
La simulación comprueba el funcionamiento del programa en el circuito virtual,
no las condiciones eléctricas de un montaje real.

[Volver a elegir ruta](#elige-tu-ruta) · [Resolver un problema](#problemas-habituales)

## 6. Carga en hardware real (Opcional) 

Esta sección permite probar el mismo firmware en un **Arduino Uno R3 físico**
desde Linux. La carga reemplaza el programa que ya tenga la placa. Usa el
firmware original `firmware/pot_led.hex`; no necesitas recompilar el modelo.

### Montar el circuito

Con el USB desconectado, reproduce el circuito virtual en una protoboard. Para
esta prueba alimenta el montaje solo con el USB del Uno y comprueba que no haya
un cortocircuito entre 5V y GND antes de conectarlo.

| Elemento | Conexión al Uno |
| --- | --- |
| Extremos del potenciómetro | 5V y GND |
| Cursor del potenciómetro | A0 |
| Salida PWM | D3 -> resistencia de 330 ohmios -> ánodo del LED |
| Cátodo del LED | GND |
| USB | Cable de datos al equipo Linux |

No conectes el LED sin resistencia. Deja libres D0/RX y D1/TX durante la carga
y cierra cualquier monitor serie u otro programa que esté usando la placa.

### Identificar el puerto

Conecta el Uno mediante un cable USB de datos. Desde la raíz del repositorio,
compara esta lista antes y después de conectarlo:

```bash
python3 scripts/cargar_hardware.py --listar-puertos
```

El resultado suele ser `/dev/ttyACM0` o `/dev/ttyUSB0`; usa el puerto que
aparece al conectar la placa. También se puede usar el enlace estable mostrado
en `/dev/serial/by-id/`. El script no selecciona un puerto automáticamente.

Si indica falta de permisos, añade tu usuario al grupo `dialout` ejecutando el siguiente comando: 

```bash
sudo usermod -aG dialout "$USER"
```

Cierra la sesión de Linux de tu computadora y vuelve a iniciarla antes de repetir el comando para listar los puertos.

### Comprobar y cargar el firmware

Sustituye `/dev/ttyUSB0` por el puerto que identificaste. Primero comprueba los
archivos y permisos sin abrir ni reiniciar la placa:

```bash
python3 scripts/cargar_hardware.py --puerto /dev/ttyUSB0 --comprobar
```

El comando debe terminar con `PREPARADO` y mostrar el archivo HEX junto con su
SHA-256. Esta comprobación no demuestra todavía la comunicación con el Uno.

Cuando estés seguro de que es la placa correcta, ejecuta la carga:

```bash
python3 scripts/cargar_hardware.py --puerto /dev/ttyUSB0
```

El asistente muestra el puerto, el firmware y el comando antes de escribir la
flash. Escribe `CARGAR` solo para confirmar; `avrdude` programa y verifica la
memoria. Al terminar debe indicar `CARGADO`. Gira el potenciómetro y comprueba
LED apagado al mínimo, brillo intermedio y máximo al girarlo; pulsa RESET y
repite la prueba. El mensaje de carga no sustituye esta verificación física.

Para cargar otro firmware HEX, por ejemplo la variante de transición suave,
indícalo explícitamente con `--firmware`:

```bash
python3 scripts/cargar_hardware.py --puerto /dev/ttyUSB0 --firmware firmware/pot_led_suave.hex
```

## Qué hace cada archivo

| Archivo | Uso |
| --- | --- |
| [README.md](README.md) | Tutorial principal y navegación entre ambas rutas |
| [iniciar.py](iniciar.py) | Comprueba el proyecto, prepara Wokwi y abre el circuito |
| [scripts/instalar_dependencias.py](scripts/instalar_dependencias.py) | Añade los productos MATLAB y el soporte Arduino necesarios en Linux |
| [scripts/verificar_entorno.m](scripts/verificar_entorno.m) | Muestra productos y complementos MATLAB disponibles |
| [model/pot_led.slx](model/pot_led.slx) | Diagrama editable del algoritmo |
| [scripts/preparar_modelo.m](scripts/preparar_modelo.m) | Crea el diagrama desde cero en una copia sin modelo |
| [scripts/compilar_modelo.m](scripts/compilar_modelo.m) | Genera y compila firmware sin programar una placa |
| [firmware/pot_led.hex](firmware/pot_led.hex) y [firmware/pot_led.elf](firmware/pot_led.elf) | Firmware listo para el simulador |
| [wokwi_emulation/diagram.json](wokwi_emulation/diagram.json) | Componentes y conexiones: 5V/GND del potenciómetro, A0, D3-resistencia-LED-GND |
| [wokwi_emulation/wokwi.toml](wokwi_emulation/wokwi.toml) | Indica a Wokwi dónde encontrar HEX y ELF |
| [.gitignore](.gitignore) | Excluye los temporales de compilación y caché |

En `wokwi.toml`, `../firmware/` significa subir desde la carpeta del circuito
hacia la carpeta `firmware` del proyecto. No es una ruta fija del computador.
La resistencia virtual de 330 ohmios está en serie con el LED. Los cables del
diagrama Wokwi representan conexiones eléctricas; las líneas del modelo
Simulink representan señales del algoritmo.

## Problemas habituales

| Mensaje o síntoma | Qué hacer |
| --- | --- |
| No se encuentra `python3` | Instala Python 3.11+ y vuelve a abrir el terminal |
| No se encuentra `code` | Habilita el comando de VS Code en PATH y vuelve a abrir el terminal; el asistente da indicaciones |
| Falta `firmware` o un archivo está vacío | Descarga el proyecto completo o [genera el firmware con MATLAB](#3-generar-firmware-solo-ruta-b) |
| Wokwi no encuentra la configuración | Abre la carpeta `wokwi_emulation` en una ventana propia, mediante el asistente Linux o manualmente en Windows |
| No se encuentra `arduinolib` en MATLAB | Repite `python3 scripts/instalar_dependencias.py` y revisa el error mostrado |
| MATLAB intenta detectar un Uno al compilar | Usa `compilar_modelo()` y comprueba que la acción del target sea `Build`, no `Build, load and run` |
| No aparecen los comandos Wokwi | Guarda tu trabajo y ejecuta `F1` > **Developer: Reload Window** |
| El LED no cambia | Verifica que la simulación arrancó, el potenciómetro llega a A0 y el LED sale de D3 a través de la resistencia |

[Volver a elegir ruta](#elige-tu-ruta)

