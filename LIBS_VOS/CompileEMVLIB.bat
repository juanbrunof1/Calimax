echo off
rem ****************************************************************
rem * COMPILACION DE EMVLIB
rem *
rem ****************************************************************

echo Emv Library
cd .\EMVLIB\project
del ..\obj\*.o
nmake Lab.smk
copy ..\output\libVosEMVProcess.so ..\..\..\lib
copy ..\include\VosEMVProcess.h  ..\..\..\inc\EmvRdc
cd ..\..
echo FIN DEL PROCESO
PAUSE








