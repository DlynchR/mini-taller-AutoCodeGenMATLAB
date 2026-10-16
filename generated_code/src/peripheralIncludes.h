/*****************************************************************************************************
To enable Connected I/O for peripherals generated using the iobuilder workflow, it is necessary to include the peripheralIncludes.h file in the corresponding peripheral's header files. 
This ensures that the codes in the driver file, which have conditional compiles, are accessible in the Connected I/O workflow.
During the build workflow, the peripheralIncludes.h file needs to be removed. 
Although there are MACROs available in External mode, Build and Deploy for scenarios like FasterRuns and FasterBuilds, there are no safeguards to exclude the peripheralIncludes.h file during the build workflow.
To prevent any errors during the build workflow, an empty peripheralIncludes.h file can be created in the includes directory of the arduinobase.
******************************************************************************************************/