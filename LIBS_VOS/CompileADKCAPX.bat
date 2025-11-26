echo off
rem ****************************************************************
rem * COMPILACION DE CAPX
rem *
rem ****************************************************************

echo CapX Library
cd .\ADK_CAPX\project
del ..\obj\*.o
nmake AdkCapX.smk
copy ..\output\libAdkCapX.so ..\..\..\lib
copy ..\inc\*.h ..\..\..\inc\Capx
cd ..\..
echo FIN DEL PROCESO
PAUSE








