function archivo = exportar_ci()
    raiz = fileparts(fileparts(mfilename('fullpath')));
    temporal = tempname;
    mkdir(temporal);
    limpiar = onCleanup(@() rmdir(temporal, 's'));
    copiaModelo = fullfile(temporal, 'modelo-original.slx');
    copyfile(fullfile(raiz, 'model', 'pot_led.slx'), copiaModelo);
    resultado = compilar_modelo(temporal);
    carpetaBuild = fullfile(temporal, 'build', 'pot_led_ert_rtw');
    informacion = fullfile(carpetaBuild, 'buildInfo.mat');
    macros = fileread(fullfile(carpetaBuild, 'arduino_macros.mk'));
    assert(~isempty(regexp(macros, '(?m)^AVR_LIB_VERSION = 1\.8\.3\r?$', 'once')) && ...
        ~isempty(regexp(macros, '(?m)^AVR_GCC_LIB_VERSION = 7\.3\.0-atmel3\.6\.1-arduino7\r?$', 'once')), ...
        'minitaller:ToolchainDistinto', ...
        'Las versiones Arduino/AVR del build no coinciden con las fijadas en CI.');
    info = loadBuildInfo(informacion);
    metadatos.matlab_release = version('-release');
    metadatos.defines = getDefines(info);
    metadatos.sources = cellfun(@nombreArchivo, getSourceFiles(info, true, true), ...
        'UniformOutput', false);
    metadatos.sources(strcmp(metadatos.sources, 'ert_main.c')) = [];
    metadatos.sources{end + 1} = 'ert_main.c';
    archivoMetadatos = fullfile(temporal, 'metadata.json');
    descriptor = fopen(archivoMetadatos, 'w');
    assert(descriptor ~= -1, 'minitaller:NoEscribible', 'No se pudo crear metadata.json.');
    cerrarArchivo = onCleanup(@() fclose(descriptor));
    fprintf(descriptor, '%s\n', jsonencode(metadatos));
    clear cerrarArchivo;
    destino = fullfile(raiz, 'build', 'ci-export');
    if ~isfolder(destino)
        mkdir(destino);
    end
    archivo = fullfile(destino, 'pot_led.zip');
    directorioOriginal = pwd;
    restaurarDirectorio = onCleanup(@() cd(directorioOriginal));
    cd(carpetaBuild);
    packNGo(informacion, 'packType', 'hierarchical', ...
        'nestedZipFiles', false, 'fileName', archivo, ...
        'minimalHeaders', true, 'includeReport', false, ...
        'ignoreParseError', false, 'ignoreFileMissing', false);
    clear restaurarDirectorio;
    argumentos = {'python3', fullfile(raiz, 'scripts', 'ci_firmware.py'), ...
        'exportar', '--raiz', raiz, '--paquete', archivo, ...
        '--metadatos', archivoMetadatos, '--modelo-copia', copiaModelo, ...
        '--referencia', resultado.hex};
    citar = @(texto) [char(39), strrep(char(texto), char(39), char([39 34 39 34 39])), char(39)];
    comando = strjoin(cellfun(citar, argumentos, 'UniformOutput', false), ' ');
    [estado, salida] = system(comando);
    fprintf('%s', salida);
    assert(estado == 0, 'minitaller:ExportacionFallida', 'No se pudo exportar el firmware para CI.');
    fprintf('Fuentes en generated_code. El firmware de la demo no fue reemplazado.\n');
end

function nombre = nombreArchivo(ruta)
    [~, base, extension] = fileparts(ruta);
    nombre = [base extension];
end