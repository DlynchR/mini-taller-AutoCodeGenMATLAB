function rutaModelo = preparar_modelo()
    % PREPARAR_MODELO Construye el diagrama A0 -> PWM D3 y devuelve su ruta.
    % Ejecutar solo al crear el modelo desde cero; no compila ni carga la placa.

    % Resolver las rutas desde este archivo evita depender de la carpeta actual.
    raiz = fileparts(fileparts(mfilename('fullpath')));
    carpetaModelo = fullfile(raiz, 'model');
    modelo = 'pot_led';
    rutaModelo = fullfile(carpetaModelo, [modelo '.slx']);

    % Proteger tanto el archivo guardado como cualquier edicion abierta.
    assert(~isfile(rutaModelo), 'minitaller:ModeloExistente', ...
        'El modelo ya existe. No se sobrescribira: %s', rutaModelo);
    assert(~bdIsLoaded(modelo), 'minitaller:ModeloAbierto', ...
        'Cierre el modelo pot_led antes de ejecutar este script.');

    % Cargar los bloques del soporte y crear un modelo nuevo en memoria.
    % onCleanup libera el modelo al salir, incluso si una instruccion falla.
    load_system('arduinolib');
    new_system(modelo);
    limpieza = onCleanup(@() close_system(modelo, 0));
    % Actualizar el algoritmo cada 10 ms; esto no fija la frecuencia del PWM.
    % StopTime es una opcion de simulacion, no una orden de apagado del Uno.
    % Habilitar el reporte C sin abrirlo automaticamente durante el build.
    set_param(modelo, 'HardwareBoard', 'Arduino Uno', ...
        'SolverType', 'Fixed-step', 'Solver', 'FixedStepDiscrete', ...
        'FixedStep', '0.01', 'StopTime', '1', 'TargetLang', 'C', ...
        'GenerateReport', 'on', 'LaunchReport', 'off');
    % El target Arduino usa por defecto "Build, load and run" en R2026b.
    % Build evita buscar hardware al terminar la compilacion del tutorial.
    configuracion = getActiveConfigSet(modelo);
    codertarget.data.setParameterValue(configuracion, ...
        'Runtime.BuildAction', 'Build');

    % ADC de 10 bits del Uno: pin analogico 0 (A0), lectura de 0 a 1023.
    % Position solo controla donde se dibuja cada bloque, no su comportamiento.
    add_block('arduinolib/Analog Input', [modelo '/Lectura_A0'], ...
        'pinNumber', '0', 'SampleTime', '0.01', ...
        'OutputDataType', 'uint16', 'Position', [40 80 140 130]);
    % Pasar a single conserva la fraccion de la ganancia antes de cuantizar.
    add_block('simulink/Signal Attributes/Data Type Conversion', ...
        [modelo '/ADC_a_single'], 'OutDataTypeStr', 'single', ...
        'Position', [190 80 280 130]);
    % Adaptar el rango del ADC al mando PWM de 8 bits: 1023 se transforma en 255.
    add_block('simulink/Math Operations/Gain', [modelo '/Escala_PWM'], ...
        'Gain', 'single(255/1023)', 'OutDataTypeStr', 'single', ...
        'Position', [330 80 430 130]);
    % Limitar la senal antes de convertirla al tipo que consume el bloque PWM.
    add_block('simulink/Discontinuities/Saturation', ...
        [modelo '/Limites_PWM'], 'LowerLimit', '0', 'UpperLimit', '255', ...
        'Position', [480 80 570 130]);
    % Floor descarta la fraccion para valores no negativos. La saturacion de
    % conversion evita un desbordamiento por vuelta al inicio del rango uint8.
    add_block('simulink/Signal Attributes/Data Type Conversion', ...
        [modelo '/PWM_a_uint8'], 'OutDataTypeStr', 'uint8', ...
        'RndMeth', 'Floor', 'SaturateOnIntegerOverflow', 'on', ...
        'Position', [620 80 710 130]);
    % D3 controla el LED; la frecuencia portadora queda en el valor del soporte.
    % Un mando 0 apaga el LED y 255 solicita la salida maxima.
    add_block('arduinolib/PWM', [modelo '/LED_D3'], ...
        'pinNumber', '3', 'frequencyType', 'Default', ...
        'Position', [760 80 860 130]);

    % Conectar los puertos de senal en el mismo orden del algoritmo.
    % Estas lineas no son los cables electricos del circuito Wokwi.
    add_line(modelo, 'Lectura_A0/1', 'ADC_a_single/1');
    add_line(modelo, 'ADC_a_single/1', 'Escala_PWM/1');
    add_line(modelo, 'Escala_PWM/1', 'Limites_PWM/1');
    add_line(modelo, 'Limites_PWM/1', 'PWM_a_uint8/1');
    add_line(modelo, 'PWM_a_uint8/1', 'LED_D3/1');

    % Guardar el diagrama editable .slx. El firmware se obtiene con otro script.
    if ~isfolder(carpetaModelo)
        mkdir(carpetaModelo);
    end
    save_system(modelo, rutaModelo);
    fprintf('Modelo creado sin compilar ni desplegar: %s\n', rutaModelo);
end