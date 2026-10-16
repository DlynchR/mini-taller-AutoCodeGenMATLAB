function rutaModelo = preparar_modelo_suave()
    raiz = fileparts(fileparts(mfilename('fullpath')));
    original = 'pot_led';
    variante = 'pot_led_suave';
    rutaOriginal = fullfile(raiz, 'model', [original '.slx']);
    rutaModelo = fullfile(raiz, 'model', [variante '.slx']);
    assert(isfile(rutaOriginal), 'minitaller:SinModelo', 'Falta el modelo pot_led original.');
    assert(~isfile(rutaModelo), 'minitaller:ModeloExistente', ...
        'No se sobrescribira el modelo existente: %s', rutaModelo);
    assert(~bdIsLoaded(original) && ~bdIsLoaded(variante), ...
        'minitaller:ModeloAbierto', 'Guarde y cierre ambos modelos antes de continuar.');

    modelo = load_system(rutaOriginal);
    cerrarModelo = onCleanup(@() close_system(modelo, 0));
    assert(strcmp(get_param(modelo, 'FixedStep'), '0.01'), ...
        'minitaller:MuestreoDistinto', 'Esta variante requiere el paso de 0.01 s.');
    delete_line(original, 'Limites_PWM/1', 'PWM_a_uint8/1');
    add_block('simulink/Discontinuities/Rate Limiter', ...
        [original '/Transicion_suave'], ...
        'RisingSlewLimit', 'single(100)', 'FallingSlewLimit', 'single(-100)', ...
        'InitialCondition', 'single(0)', 'SampleTimeMode', 'inherited', ...
        'Position', [620 80 720 130]);
    set_param([original '/PWM_a_uint8'], 'Position', [770 80 860 130]);
    set_param([original '/LED_D3'], 'Position', [910 80 1010 130]);
    add_line(original, 'Limites_PWM/1', 'Transicion_suave/1');
    add_line(original, 'Transicion_suave/1', 'PWM_a_uint8/1');
    save_system(modelo, rutaModelo);
    fprintf('Variante creada: %s\n', rutaModelo);
    fprintf('El archivo pot_led.slx y sus firmwares no se modificaron.\n');
end