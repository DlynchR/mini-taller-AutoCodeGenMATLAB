function test_transicion_suave()
    raiz = fileparts(fileparts(mfilename('fullpath')));
    variante = 'pot_led_suave';
    ruta = fullfile(raiz, 'model', [variante '.slx']);
    assert(isfile(ruta), 'minitaller:SinVariante', 'Ejecute preparar_modelo_suave primero.');
    assert(~bdIsLoaded(variante), 'minitaller:ModeloAbierto', 'Guarde y cierre la variante.');
    modelo = load_system(ruta);
    cerrarModelo = onCleanup(@() close_system(modelo, 0));
    prueba = 'prueba_transicion_suave';
    assert(~bdIsLoaded(prueba), 'minitaller:PruebaAbierta', 'Cierre el modelo de prueba anterior.');
    new_system(prueba);
    cerrarPrueba = onCleanup(@() close_system(prueba, 0));
    assert(strcmp(get_param(modelo, 'FixedStep'), '0.01'), ...
        'minitaller:MuestreoDistinto', 'El periodo del modelo debe ser 10 ms.');
    set_param(prueba, 'SolverType', 'Fixed-step', 'Solver', 'FixedStepDiscrete', ...
        'FixedStep', '0.01');
    add_block('simulink/Sources/From Workspace', [prueba '/ADC'], ...
        'VariableName', 'entrada_prueba', 'SampleTime', '0.01', 'Interpolate', 'off', ...
        'OutputAfterFinalValue', 'Holding final value');
    add_block('simulink/Signal Attributes/Data Type Conversion', [prueba '/ADC_uint16'], ...
        'OutDataTypeStr', 'uint16');
    add_line(prueba, 'ADC/1', 'ADC_uint16/1');
    bloques = {'ADC_a_single', 'Escala_PWM', 'Limites_PWM', 'Transicion_suave', 'PWM_a_uint8'};
    anterior = 'ADC_uint16';
    for indice = 1:numel(bloques)
        nombre = bloques{indice};
        add_block([variante '/' nombre], [prueba '/' nombre]);
        add_line(prueba, [anterior '/1'], [nombre '/1']);
        anterior = nombre;
    end
    add_block('simulink/Sinks/To Workspace', [prueba '/PWM'], ...
        'VariableName', 'pwm_prueba', 'SaveFormat', 'Array');
    add_line(prueba, 'PWM_a_uint8/1', 'PWM/1');

    casos = {zeros(300, 1), repmat(1023, 300, 1), ...
        [repmat(1023, 300, 1); zeros(300, 1)], repmat(512, 300, 1), ...
        [zeros(10, 1); repmat(1023, 80, 1); zeros(40, 1); repmat(512, 300, 1)]};
    for caso = 1:numel(casos)
        adc = casos{caso};
        tiempo = (0:numel(adc) - 1)' * 0.01;
        entrada = Simulink.SimulationInput(prueba);
        entrada = entrada.setVariable('entrada_prueba', [tiempo adc]);
        entrada = entrada.setModelParameter('StopTime', num2str(tiempo(end)), ...
            'ReturnWorkspaceOutputs', 'on');
        resultado = sim(entrada);
        pwm = double(resultado.pwm_prueba(:));
        esperado = zeros(size(adc));
        previo = single(0);
        for muestra = 1:numel(adc)
            objetivo = single(adc(muestra)) * single(255 / 1023);
            previo = previo + min(max(objetivo - previo, single(-1)), single(1));
            esperado(muestra) = floor(double(previo));
        end
        assert(isequal(pwm, esperado), 'minitaller:RampaIncorrecta', ...
            'El caso %d no coincide con la rampa de 100 niveles por segundo.', caso);
        assert(all(abs(diff([0; pwm])) <= 1) && all(pwm >= 0 & pwm <= 255), ...
            'minitaller:LimiteIncorrecto', 'Se excedio el incremento o el rango PWM.');
    end
    fprintf('OK: 5 casos de transicion suave, sin bloques de hardware ni acceso al Uno.\n');
end