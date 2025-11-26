@echo off
@echo ##########################################################################################################################
@echo Script  /  AppName  /  AppVer  /  Platform  / PackType /  Compile (YES / NO)
@echo ##########################################################################################################################
call  C:\VDE\Python27\python.exe FW_RDC.py Elavon 1.2.2 VOS2 PACK2 YES DEV
pause
call  C:\VDE\Python27\python.exe FW_RDC.py Elavon 1.2.2 VOS2 PACK2 YES PROD
