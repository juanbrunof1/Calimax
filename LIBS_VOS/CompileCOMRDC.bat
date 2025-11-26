echo off
rem ****************************************************************
rem * COMPILACION DE COMM RDC
rem *
rem ****************************************************************


echo Comms Library
cd  .\COMRDC\project
del  ..\obj\*.o
nmake Lab.smk
copy  ..\output\libComRdc.so ..\..\..\lib
copy  ..\include\*.h  ..\..\..\inc\ComRdc
copy  ..\include\*.h   ..\..\EMVLIB\include
cd ..\..
echo FIN DEL PROCESO
PAUSE








