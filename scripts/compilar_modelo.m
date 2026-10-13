function resultado = compilar_modelo()
    raiz = fileparts(fileparts(mfilename('fullpath')));
    modelo = 'pot_led';
    rutaModelo = fullfile(raiz, 'model', [modelo '.slx']);
    carpetaBuild = fullfile(raiz, 'build');
    carpetaFirmware = fullfile(raiz, 'firmware');

    assert(isfile(rutaModelo), 'minitaller:SinModelo', ...
        'Ejecute preparar_modelo antes de compilar.');
    assert(~bdIsLoaded(modelo), 'minitaller:ModeloAbierto', ...
        'Guarde y cierre pot_led antes de compilar con este script.');

    if ~isfolder(carpetaBuild)
        mkdir(carpetaBuild);
    end
    directorioOriginal = pwd;
    restaurarDirectorio = onCleanup(@() cd(directorioOriginal));
    configuracionOriginal = Simulink.fileGenControl('getConfig');
    restaurarConfiguracion = onCleanup(@() Simulink.fileGenControl( ...
        'setConfig', 'config', configuracionOriginal));
    Simulink.fileGenControl('set', 'CacheFolder', ...
        fullfile(carpetaBuild, 'cache'), 'CodeGenFolder', carpetaBuild, ...
        'createDir', true);
    cd(carpetaBuild);

    load_system(rutaModelo);
    cerrarModelo = onCleanup(@() close_system(modelo, 0));
    assert(strcmp(get_param(modelo, 'HardwareBoard'), 'Arduino Uno'), ...
        'minitaller:PlacaIncorrecta', 'El firmware debe ser para Arduino Uno.');
    configuracion = getActiveConfigSet(modelo);
    codertarget.data.setParameterValue(configuracion, ...
        'Runtime.BuildAction', 'Build');
    datosTarget = codertarget.data.getData(configuracion);
    assert(strcmp(datosTarget.Runtime.BuildAction, 'Build'), ...
        'minitaller:DespliegueNoPermitido', ...
        'La accion de construccion debe ser Build, sin carga a la placa.');
    set_param(modelo, 'GenCodeOnly', 'off', 'LaunchReport', 'off', ...
        'RTWVerbose', 'off');

    slbuild(modelo, 'StandaloneCoderTarget', 'GenerateCodeOnly', false);

    archivosHex = dir(fullfile(carpetaBuild, '**', [modelo '.hex']));
    archivosElf = dir(fullfile(carpetaBuild, '**', [modelo '.elf']));
    assert(numel(archivosHex) == 1 && numel(archivosElf) == 1, ...
        'minitaller:ArtefactosInesperados', ...
        'Se esperaba un HEX y un ELF; se encontraron %d y %d en %s.', ...
        numel(archivosHex), numel(archivosElf), carpetaBuild);

    if ~isfolder(carpetaFirmware)
        mkdir(carpetaFirmware);
    end
    resultado.hex = fullfile(carpetaFirmware, [modelo '.hex']);
    resultado.elf = fullfile(carpetaFirmware, [modelo '.elf']);
    copyfile(fullfile(archivosHex.folder, archivosHex.name), resultado.hex);
    copyfile(fullfile(archivosElf.folder, archivosElf.name), resultado.elf);
    fprintf('Firmware compilado, pendiente de emular:\n%s\n%s\n', ...
        resultado.hex, resultado.elf);
end