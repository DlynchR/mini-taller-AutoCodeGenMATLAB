# Fuentes de la aplicacion pot_led

Esta carpeta contiene exclusivamente fuentes necesarias para compilar la
aplicacion educativa generada por Simulink. No es una distribucion del soporte
Arduino de MathWorks ni una biblioteca para usos independientes.

Las fuentes generadas conservan su aviso Academic License. Las fuentes de
soporte conservan los avisos originales de MathWorks; no se relicencian bajo la
licencia del repositorio. El contrato de MathWorks R2026b, Parte Dos, seccion
3.2 (Coder Programs), contempla la copia, modificacion y despliegue de archivos
de soporte necesarios junto con codigo generado como parte de una aplicacion
autonoma. Esto no concede permisos generales sobre otros archivos del producto.
Consultar los terminos aplicables a la propia licencia antes de otros usos.

El nucleo Arduino AVR y el compilador se descargan desde Arduino, con versiones
y hashes fijos, y no se incorporan aqui. Conservan sus propias licencias, que
estan incluidas en sus distribuciones. Los artefactos de CI incluyen el nucleo
y sus avisos para permitir reconstruir el ejecutable con versiones modificadas.

No editar `src` a mano. Regenerar con `exportar_ci()` desde MATLAB. El manifiesto
detecta cambios accidentales del modelo o de las fuentes; no es una firma
criptografica que sustituya la revision de un pull request.