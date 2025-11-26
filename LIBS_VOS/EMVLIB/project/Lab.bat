@echo off
rem ******************************************************************
rem This batch file is used to setup Raptor projects and create
rem bundles and packages.
rem ******************************************************************
cls

rem Set our paths
SET VOS2_TOOLCHAIN=\vde\plugins\com.verifone.toolchain.linaro.win32_1.0.0.201507081257\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
SET VOS2_SDK=\vde\plugins\com.verifone.sdk.VOS2_50070101_1.0.0\vos2
SET VFI_ADK=\vde\plugins\com.verifone.adk.adk200_104_1.0.0\adk-full-2.0.0-104\vos2
rem SET VFI_ADK=\raptor\adk-full-rap-cd2-6.1\vos2

rem Backup original Path
SET PATH_ORIG=%PATH%

rem Set lab info
SET LAB_NAME=DLL_VOS2
SET LAB_VERSION=1.0.0
SET LAB_USER=usr1

if "%1" == "config" goto CONFIG_DIR
if "%1" == "CONFIG" goto CONFIG_DIR
if "%1" == "/c" goto CONFIG_DIR
if "%1" == "/C" goto CONFIG_DIR

if "%1" == "clean" goto CLEAN_PRJ
if "%1" == "CLEAN" goto CLEAN_PRJ
if "%1" == "Clean" goto CLEAN_PRJ

if "%1" == "clean-all" goto CLEAN_ALL
if "%1" == "CLEAN-ALL" goto CLEAN_ALL
if "%1" == "Clean-All" goto CLEAN_ALL


if "%1" == "build" goto BUILD_PRJ
if "%1" == "BUILD" goto BUILD_PRJ
if "%1" == "Build" goto BUILD_PRJ
if "%1" == "/B" goto BUILD_PRJ
if "%1" == "/b" goto BUILD_PRJ

if "%1" == "install" goto INSTALL
if "%1" == "INSTALL" goto INSTALL
if "%1" == "Install" goto INSTALL
if "%1" == "/I" goto INSTALL
if "%1" == "/i" goto INSTALL

goto EXIT

:BUILD_PRJ
SET PATH=%PATH%;%VOS2_TOOLCHAIN%
nmake Lab.smk
goto EXIT

:CLEAN_ALL
cd ..\output
if exist "pkg" (
	cd pkg
	del /Q /S *
	del /Q *.*
	cd ..
	rmdir /S /Q pkg
)
if exist "bundle" (
	cd bundle
	del /Q /S *
	del /Q *.*
	cd ..
	rmdir /S /Q bundle
)
if exist "vos2pkg" (
	cd vos2pkg
	del /Q /S *
	del /Q *.*
	cd ..
	rmdir /S /Q vos2pkg
)
cd ..\project
:CLEAN_PRJ
nmake /f Lab.smk clean
goto EXIT

:INSTALL
rem Create pacakge folder
cd ..\output
if not exist "pkg" mkdir pkg
cd pkg
if not exist "CONTROL" mkdir CONTROL
rem Create control file
echo Creating control file...
echo Name: %LAB_NAME% > CONTROL\control
echo Version: %LAB_VERSION% >> CONTROL\control
echo User: %LAB_USER% >> CONTROL\control
rem Create start file
echo Creating start file...
echo %LAB_NAME%.exe> CONTROL\start
rem echo.>> CONTROL\start
cd ..
rem Copy binary file
echo Copying files...
if not exist lab goto EXIT_ERROR
cp lab pkg\%LAB_NAME%.exe
cp ../project/syslog.conf pkg
rem Copying libs... - STRK: Not quite sure of it
rem if not exist "pkg\lib" mkdir pkg\lib
rem xcopy /E /Y "..\project\lib" "pkg\lib"
rem TAR package
cd pkg
echo About to pkg...
tar -cvf pkg.%LAB_NAME%.tar *
cd ..

rem Create bundle folder
if not exist "bundle" mkdir bundle
cd bundle
if not exist "crt" mkdir crt
if not exist "CONTROL" mkdir CONTROL
echo Name: %LAB_NAME% > CONTROL\control
echo Version: %LAB_VERSION% >> CONTROL\control
echo User: %LAB_USER% >> CONTROL\control
cd ..
rem Move Package TAR
mv pkg/pkg.%LAB_NAME%.tar bundle
rem TGZ Bundle
cd bundle
tar -cvzf %LAB_USER%.bundle.%LAB_NAME%.tgz *
tar -cf dl.%LAB_NAME%-vos2.tar %LAB_USER%.bundle.%LAB_NAME%.tgz
cd ..

if not exist "vos2pkg" mkdir vos2pkg
mv bundle/dl.%LAB_NAME%-vos2.tar vos2pkg
del bundle\%LAB_USER%.bundle.%LAB_NAME%.tgz
cd ..\project
goto EXIT

:CONFIG_DIR
echo Creating directories...
if not exist "output" mkdir output
if not exist "obj" mkdir obj

echo Creating sym-links...
if not exist "vfiadk" mklink /J vfiadk %VFI_ADK%
if not exist "vfisdk" mklink /J vfisdk %VOS2_SDK%

goto EXIT

:USAGE

:EXIT_ERROR
echo ERROR!

:EXIT
SET PATH=%PATH_ORIG%
SET VOS2_TOOLCHAIN=
SET VOS2_SDK=
SET VFI_ADK=
SET LAB_NAME=
pause
