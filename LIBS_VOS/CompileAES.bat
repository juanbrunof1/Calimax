echo off
rem ****************************************************************
rem * COMPILACION DE AES
rem *
rem ****************************************************************


echo AES Library
cd .\AES_ENCRYP\project
del ..\obj\*.o
nmake aes.smk
copy ..\output\libAES_encryp.so ..\..\..\lib
copy ..\include\*.h ..\..\..\inc\App
cd ..\..
echo FIN DEL PROCESO
PAUSE








