function estado = verificar_entorno()
    % VERIFICAR_ENTORNO Describe los productos y complementos de esta sesion.
    % Devuelve una estructura para consultar resultados sin analizar texto.
    % No instala productos, solicita licencias ni ejecuta una compilacion.

    % Consultar ver con salida evita imprimir la cabecera con datos de licencia.
    productos = ver;
    nombres = string({productos.Name});

    % Mostrar versiones permite registrar el entorno que produjo el firmware.
    fprintf('MATLAB: %s (%s)\n', version, version('-release'));
    fprintf('Instalacion: %s\n', matlabroot);
    fprintf('\nProductos instalados:\n');
    for indice = 1:numel(productos)
        fprintf('  %s: %s\n', productos(indice).Name, productos(indice).Version);
    end

    % Una carpeta en disco no garantiza que MATLAB reconozca el producto.
    % Para Simulink se comprueba tambien que su entrada sea localizable.
    estado.simulinkInstalado = any(nombres == "Simulink");
    estado.simulinkCoderInstalado = any(nombres == "Simulink Coder");
    estado.embeddedCoderInstalado = any(nombres == "Embedded Coder");
    estado.entradaSimulink = which('simulink');
    estado.simulinkDisponible = estado.simulinkInstalado && ...
        ~isempty(estado.entradaSimulink);

    % Los valores 1 y 0 indican presencia, no una prueba de uso de la licencia.
    fprintf('\nSimulink registrado y localizable: %d\n', estado.simulinkDisponible);
    fprintf('Simulink Coder instalado: %d\n', estado.simulinkCoderInstalado);
    fprintf('Embedded Coder instalado: %d\n', estado.embeddedCoderInstalado);
    fprintf('Estas comprobaciones no verifican licencias ni una compilacion.\n');

    % El soporte Arduino se identifica en esta tabla cuando esta registrado.
    % Se muestran solo nombre, version y habilitacion, no otros identificadores.
    fprintf('\nComplementos registrados:\n');
    estado.complementos = matlab.addons.installedAddons;
    if isempty(estado.complementos)
        fprintf('  No se encontraron complementos registrados.\n');
    else
        disp(estado.complementos(:, {'Name', 'Version', 'Enabled'}));
    end
end