function estado = verificar_entorno()
    productos = ver;
    nombres = string({productos.Name});

    fprintf('MATLAB: %s (%s)\n', version, version('-release'));
    fprintf('Instalacion: %s\n', matlabroot);
    fprintf('\nProductos instalados:\n');
    for indice = 1:numel(productos)
        fprintf('  %s: %s\n', productos(indice).Name, productos(indice).Version);
    end

    estado.simulinkInstalado = any(nombres == "Simulink");
    estado.simulinkCoderInstalado = any(nombres == "Simulink Coder");
    estado.embeddedCoderInstalado = any(nombres == "Embedded Coder");
    estado.entradaSimulink = which('simulink');
    estado.simulinkDisponible = estado.simulinkInstalado && ...
        ~isempty(estado.entradaSimulink);

    fprintf('\nSimulink registrado y localizable: %d\n', estado.simulinkDisponible);
    fprintf('Simulink Coder instalado: %d\n', estado.simulinkCoderInstalado);
    fprintf('Embedded Coder instalado: %d\n', estado.embeddedCoderInstalado);
    fprintf('Estas comprobaciones no verifican licencias ni una compilacion.\n');

    fprintf('\nComplementos registrados:\n');
    estado.complementos = matlab.addons.installedAddons;
    if isempty(estado.complementos)
        fprintf('  No se encontraron complementos registrados.\n');
    else
        disp(estado.complementos(:, {'Name', 'Version', 'Enabled'}));
    end
end