echo off
rem ****************************************************************
rem * COMPILACION DE CMD
rem *
rem ****************************************************************

echo Command Library
cd .\CMDPP\project
del ..\obj\*.o
nmake cmdpp.smk
copy ..\output\libCmdPP.so ..\..\..\lib
copy ..\include\*.h ..\..\..\inc\PinPad
cd ..\..
echo FIN DEL PROCESO
PAUSE








