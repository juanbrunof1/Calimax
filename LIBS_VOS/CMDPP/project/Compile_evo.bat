SET VFI_ARM_EVO=1
SET PATH=C:\Program Files\ARM\bin\win_32-pentium;C:\Program Files\ARM\Utilities\FLEXlm\10.8.5.0\1\win_32-pentium;C:\Program Files\ARM\Utilities\FLEXlm\9.2\release\win_32-pentium;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;C:\Program Files\ARM\RVI\Tools\3.3\106\programs\win_32-pentium;C:\Program Files\ARM\RVI\GDB\3.3\8;C:\Program Files\Gemalto\Classic Client\BIN;C:\VerixAps\VFSDK\Bin;"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin"
IF %VFI_ARM_EVO%. == 1. (
SET PATH=%PATH%;C:\Program Files\ARM\RVCT\Programs\4.0\902\multi1\win_32-pentium;C:\Program Files\ARM\RVD\Core\4.0\1106\win_32-pentium\bin;C:\eVoAps\Tools;
) ELSE (
SET PATH=%PATH%;C:\Program Files\ARM\RVCT\Programs\2.0.1\277\win_32-pentium;C:\Program Files\ARM\RVD\Core\1.7\283\win_32-pentium\bin;C:\VerixAps\Tools;
)
NMAKE /i /f MakefileEVO_1_CMDPP.mk /a
pause