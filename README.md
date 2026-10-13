# mini-taller-AutoCodeGenMATLAB

Mini-taller de generacion automatica de codigo con Simulink: tutorial virtual
en Wokwi para VS Code y demostracion con Arduino Uno R3 fisico.

Estado: entorno MATLAB instalado, modelo generado y firmware compilado sin placa.
La configuracion de Wokwi paso validacion estatica y el expositor confirmo las
cinco pruebas interactivas el 12 de octubre de 2026. GitHub Actions y la
demostracion fisica todavia no estan preparados.

- [Guia provisional explicada para la primera defensa](docs/guia-provisional.md).
- [Version PDF de la guia provisional](docs/guia-provisional.pdf).
- [Bitacora de ejecucion y pasos reproducibles](docs/bitacora.md).
- [Comprobacion del entorno MATLAB](scripts/verificar_entorno.m).

La parte MATLAB y el pipeline de GitHub Actions se preparan antes de clase.
Los participantes realizan el tutorial virtual sin necesitar una placa fisica.

## Primera ejecucion virtual

Requisitos: VS Code, conexion a Internet, extension Wokwi y una activacion valida.
La extension instalada durante la preparacion es `wokwi.wokwi-vscode` 4.0.0.
No se presupone una licencia gratuita permanente ni que todos los participantes
tengan ya una cuenta. No se necesita MATLAB para ejecutar el firmware entregado.

1. Abrir la carpeta completa del repositorio en VS Code.
2. Si falta la extension, instalar **Wokwi Simulator** del editor Wokwi desde
	Extensions. Alternativamente, ejecutar `code --install-extension wokwi.wokwi-vscode`.
3. Presionar `F1` y seleccionar **Wokwi: Request a new License**. Completar la
	activacion directamente en el navegador y VS Code. No publicar claves, tokens
	ni datos de cuenta. Si solicita un pago, detenerse y revisar las opciones de
	acceso antes de continuar.
4. Presionar `F1`, seleccionar **Wokwi: Select Config File** y elegir
	[wokwi_simulation/wokwi.toml](wokwi_simulation/wokwi.toml). La configuracion esta
	en una subcarpeta, no en la raiz del repositorio.
5. Presionar `F1` y ejecutar **Wokwi: Start Simulator**. Deben aparecer el Uno,
	el potenciometro, la resistencia y el LED del
	[circuito](wokwi_simulation/diagram.json).
6. Mover el potenciometro desde el minimo hasta el maximo y comprobar el LED.
	Detener y volver a iniciar la simulacion para comprobar que se puede repetir.

Si no aparecen los comandos tras instalar la extension, usar **Developer: Reload
Window**, preservando antes cualquier trabajo sin guardar.

| Prueba | Resultado esperado | Estado |
| --- | --- | --- |
| Potenciometro al minimo | LED apagado | Confirmado por el expositor |
| Posicion intermedia | LED con brillo intermedio | Confirmado por el expositor |
| Potenciometro al maximo | LED con brillo maximo | Confirmado por el expositor |
| Regreso al minimo | LED vuelve a apagarse | Confirmado por el expositor |
| Reinicio del simulador | Se repite el comportamiento | Confirmado por el expositor |

Esta validacion es manual, reportada por el expositor; no es una prueba
automatizada ni una medicion con instrumentos. Repetirla al modificar el modelo
o el firmware.

El modelo actual no envia datos UART. No se espera ver valores en el Monitor
Serie todavia. La instrumentacion y las pruebas de CI son etapas posteriores.
La inspeccion visual del brillo no mide el ciclo de trabajo ni la temporizacion.

## Modelo y firmware preparados

- [Modelo Simulink](model/pot_led.slx): Uno R3, A0, PWM D3 y paso discreto de 10 ms.
- [Firmware HEX](firmware/pot_led.hex) y [firmware ELF](firmware/pot_led.elf):
  generados y compilados con R2026b para ATmega328P a 16 MHz.
- Resistencia virtual del LED: 330 ohmios. Antes de la demo fisica se comprobara
  el valor del componente real y el montaje con el expositor.

El escalamiento usa precision simple: ADC de 0 a 1023, ganancia `255/1023`,
limites de 0 a 255 y conversion a `uint8` con truncamiento. El periodo del modelo
no es la frecuencia de la portadora PWM. No se ha medido aun ninguna de las dos
temporizaciones en emulacion.

## Reproducir la preparacion en MATLAB

Estos pasos son del expositor. La instalacion y los comandos de terminal usados
estan documentados en la [bitacora](docs/bitacora.md).

Desde la raiz del repositorio en MATLAB:

```matlab
addpath(fullfile(pwd, 'scripts'));
estado = verificar_entorno();
```

El [generador](scripts/preparar_modelo.m) permite reconstruir el modelo en una
copia de trabajo donde aun no exista. Por seguridad, se niega a sobrescribir un
modelo existente. No es necesario ejecutarlo si se usa el modelo incluido:

```matlab
rutaModelo = preparar_modelo();
```

Guardar y cerrar el modelo antes de ejecutar el
[script de compilacion](scripts/compilar_modelo.m):

```matlab
resultado = compilar_modelo();
```

El script fuerza `Runtime.BuildAction = 'Build'`, verifica esa opcion y despues
genera y compila. No usar **Build, Deploy & Start** para el tutorial virtual.
Los temporales y el codigo generado localmente quedan en la carpeta `build`,
excluida de Git. Los binarios se copian a la carpeta `firmware` solo tras un build
correcto. Su compilacion no sustituye las pruebas de funcionamiento.

## Licencias y distribucion

La [licencia MIT](LICENSE) del repositorio no sustituye los terminos aplicables
a MATLAB, Wokwi ni sus dependencias. El codigo generado en esta instalacion
contiene una cabecera de **Academic License**. Antes de publicar una exportacion
de fuentes o dependencias para CI, se revisaran sus condiciones de distribucion;
no se consideraran automaticamente cubiertas por MIT ni se eliminaran avisos.