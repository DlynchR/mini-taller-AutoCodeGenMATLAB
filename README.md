# Mini-taller: Simulink y Arduino virtual

El asistente y el recorrido completo estan preparados para **Linux x86-64**.

Controla el brillo de un LED con un potenciometro en un Arduino Uno emulado.
Elige tu recorrido:

- **Rapido (sin MATLAB):** Utiliza el firmware incluido y sigue los pasos de abajo.
- **Completo (con MATLAB):** Consulta la [guia detallada](docs/tutorial-detallado.md)
  para instalar las dependencias automaticamente, examinar el modelo, generar
  codigo, compilar el firmware y probarlo en Wokwi.

**Nota para Windows:** El recorrido rapido tambien se puede hacer en Windows
con el firmware incluido. Instala manualmente la extension Wokwi en VS Code y
abre la carpeta `wokwi_emulation` como carpeta de trabajo. Desde alli, sigue
los pasos de activacion y simulacion de la seccion 2; el asistente de la
seccion 1 esta preparado para Linux.

Ambos recorridos terminan con el mismo circuito virtual. No necesitas una placa fisica.

## 1. Preparar

Necesitas **Python 3.11 o posterior, VS Code e Internet**. Wokwi requiere una
activacion valida; no se presupone una licencia gratuita permanente.

Clona el repositorio en tu equipo o descarga el repositorio completo con **Code > Download ZIP**, extraelo y abre
esa carpeta en VS Code (**File > Open Folder**). Puedes guardarla o renombrarla
como prefieras. Debes ver [iniciar.py](iniciar.py) junto a las carpetas del proyecto.

Abre **Terminal > New Terminal** en esa ventana y ejecuta:

```bash
python3 iniciar.py
```

El asistente comprueba el firmware y el circuito, instala Wokwi si falta y abre
**otra ventana de VS Code dedicada al circuito**. No modifica el modelo, los
binarios ni tus licencias. Conserva la ventana original para repetir el comando.

Resultado esperado: `PREPARADO`. Esto no significa que la simulacion ya este
ejecutandose. Si aparece `NO LISTO`, sigue la indicacion y repite el comando.

## 2. Simular

En la **nueva ventana**:

1. Si VS Code pide confianza, confirma.
2. Si aun no activaste Wokwi: `F1` > **Wokwi: Request a New License** y completa
   el acceso en el navegador. No compartas claves. Si solicita pago, consulta
  las condiciones del servicio antes de continuar. Si ya tienes activacion,
  omite este paso.
3. `F1` > **Wokwi: Start Simulator**.

No debes escribir rutas ni seleccionar el archivo de configuracion: la nueva
ventana abre directamente la carpeta que lo contiene. Deben aparecer el Uno,
el potenciometro y el LED con su resistencia.

## 3. Comprobar

| Accion | Resultado esperado |
| --- | --- |
| Potenciometro al minimo | LED apagado |
| Posicion intermedia | Brillo intermedio |
| Potenciometro al maximo | Brillo maximo |
| Volver al minimo | LED apagado otra vez |
| Detener y reiniciar | El comportamiento se repite |

El Monitor Serie puede estar vacio: el firmware actual no envia datos UART.
Ver el brillo cambiar no mide la frecuencia PWM ni valida el circuito fisico.

## Si algo falta

- Python no se encuentra: instala Python 3.11+ y abre un terminal nuevo.
- `code` no se encuentra: sigue la indicacion del asistente para habilitar el
  comando de VS Code y abre un terminal nuevo.
- Falta firmware: extrae el repositorio completo o usa la ruta MATLAB de la
  [guia detallada](docs/tutorial-detallado.md) para generarlo.
- No aparecen los comandos Wokwi: guarda tu trabajo y ejecuta
  **Developer: Reload Window** desde `F1`.

Para revisar sin instalar ni abrir ventanas, agrega `--comprobar` al comando.
Para preparar sin abrir la ventana, agrega `--sin-abrir`.

## Para entender cada paso

La [guia detallada](docs/tutorial-detallado.md) explica el circuito, cada bloque
de Simulink, el papel de los archivos y las opciones para repetir la generacion.
La [licencia del repositorio](LICENSE) no sustituye los terminos de MATLAB y Wokwi.