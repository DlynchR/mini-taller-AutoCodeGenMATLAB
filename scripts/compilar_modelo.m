function resultado = compilar_modelo(carpetaSalida, modelo)
    % COMPILAR_MODELO Genera C y enlaza firmware del Uno sin cargar una placa.
    % Devuelve las rutas HEX y ELF. Requiere el modelo guardado y cerrado.
    % Una compilacion correcta no demuestra por si sola el funcionamiento.

    % Usar rutas del repositorio, no rutas dependientes del directorio de trabajo.
    raiz = fileparts(fileparts(mfilename('fullpath')));
    if nargin < 2
        modelo = 'pot_led';
    end
    assert(isvarname(modelo), 'minitaller:NombreInvalido', 'Indique un nombre de modelo valido.');
    rutaModelo = fullfile(raiz, 'model', [modelo '.slx']);
    if nargin < 1 || isempty(carpetaSalida)
        carpetaSalida = raiz;
    end
    carpetaBuild = fullfile(carpetaSalida, 'build');
    carpetaFirmware = fullfile(carpetaSalida, 'firmware');

    % Evitar compilar sin modelo o descartar ediciones abiertas del usuario.
    assert(isfile(rutaModelo), 'minitaller:SinModelo', ...
        'Cree y guarde el modelo antes de compilar: %s', rutaModelo);
    assert(~bdIsLoaded(modelo), 'minitaller:ModeloAbierto', ...
        'Guarde y cierre %s antes de compilar con este script.', modelo);

    % Los temporales y el C generado quedan en build, excluido de Git.
    if ~isfolder(carpetaBuild)
        mkdir(carpetaBuild);
    end
    % Restaurar la carpeta de trabajo y las opciones globales de generacion
    % al salir de la funcion, incluso cuando falle el build.
    directorioOriginal = pwd;
    restaurarDirectorio = onCleanup(@() cd(directorioOriginal));
    configuracionOriginal = Simulink.fileGenControl('getConfig');
    restaurarConfiguracion = onCleanup(@() Simulink.fileGenControl( ...
        'setConfig', 'config', configuracionOriginal));
    % Separar la cache de Simulink del modelo fuente y los binarios entregables.
    Simulink.fileGenControl('set', 'CacheFolder', ...
        fullfile(carpetaBuild, 'cache'), 'CodeGenFolder', carpetaBuild, ...
        'createDir', true);
    cd(carpetaBuild);

    % Cargar la version guardada; cerrar sin guardar los ajustes de esta sesion.
    load_system(rutaModelo);
    cerrarModelo = onCleanup(@() close_system(modelo, 0));
    assert(strcmp(get_param(modelo, 'HardwareBoard'), 'Arduino Uno'), ...
        'minitaller:PlacaIncorrecta', 'El firmware debe ser para Arduino Uno.');
    % slbuild tambien ejecuta hooks del target: StandaloneCoderTarget por si
    % solo no impide descargar. Forzar Build en Runtime y verificarlo antes.
    configuracion = getActiveConfigSet(modelo);
    codertarget.data.setParameterValue(configuracion, ...
        'Runtime.BuildAction', 'Build');
    datosTarget = codertarget.data.getData(configuracion);
    assert(strcmp(datosTarget.Runtime.BuildAction, 'Build'), ...
        'minitaller:DespliegueNoPermitido', ...
        'La accion de construccion debe ser Build, sin carga a la placa.');
    % Solicitar codigo y ejecutable, sin abrir el reporte ni un log muy extenso.
    set_param(modelo, 'GenCodeOnly', 'off', 'LaunchReport', 'off', ...
        'RTWVerbose', 'off');

    % Simulink Coder genera C; el toolchain AVR compila y enlaza el modelo,
    % sus controladores, el scheduler y el core Arduino en un programa completo.
    slbuild(modelo, 'StandaloneCoderTarget', 'GenerateCodeOnly', false);

    % Exigir artefactos unicos evita elegir silenciosamente un archivo ambiguo.
    % HEX contiene la imagen de programa; ELF conserva secciones y simbolos.
    archivosHex = dir(fullfile(carpetaBuild, '**', [modelo '.hex']));
    archivosElf = dir(fullfile(carpetaBuild, '**', [modelo '.elf']));
    assert(numel(archivosHex) == 1 && numel(archivosElf) == 1, ...
        'minitaller:ArtefactosInesperados', ...
        'Se esperaba un HEX y un ELF; se encontraron %d y %d en %s.', ...
        numel(archivosHex), numel(archivosElf), carpetaBuild);

    % Publicar los binarios para Wokwi solo despues de un build exitoso.
    % Una recompilacion reemplaza las copias anteriores: repetir las pruebas.
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