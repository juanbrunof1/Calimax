#
# Paths
#

VRXSDK		= C:\eVoAps\SDK\3.9.7
#ADK 		= C:\vde\ADKs\adk-full-4.4.13-440\vrx
ADK 		= C:\vde\ADKs\adk-full-4.4.15-498\vrx
TOOLCHAIN 	= C:\eVoAps\SDK\3.9.7
EOS			= C:\eVoAps\EOSSDK\2.8.12

COMM_COMPILER 	= \bin\vrxcc.exe	
linkOptions 	= -v -vsolib -g -p -map -symbols -xref 

Tarjet 		= libAES_encryp

Project = C:\BnteBnrgoPPVOS2_EVO\PINpadVOS2\LIBS_VOS\AES_ENCRYP

SrcDir 	= $(Project)\src
ObjDir 	= $(Project)\obj
OutDir 	= $(Project)\output
IncDir  = $(Project)\include

# Include Paths
SDKIncludes		= -I$(VRXSDK)\include
ADKIncludes		= -I$(ADK)\include
EOSIncludes  	= -I$(EOS)\include
APPIncludes		= -I$(IncDir)

HARDCODERESP=-DHRD_COD_RSP
#HARDCODERESP=
SYMBOLS_APP = $(HARDCODERESP)
# for release version change the COptions to 
COptions = -vsolib -c -p -g -O2 -armcc,--exceptions -armcc,--diag-suppress=1  -armcc,--diag-suppress=1300 -armcc,--diag-suppress=1299 -armcc,--diag-suppress=177 $(SYMBOLS_APP) -DLOGAPI_ENABLE_DEBUG -D_DEBUG -DVFI_IPC_DLL_IMPORT -DVFI_SYSINFO_DLL_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_MAC_DLL_IMPORT -D_VRXEVO -DVFI_COM_DLL_IMPORT -D_EXCEPTION_LOGGING -DLIB_CONFIG -DVFI_GUIPRT_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_POSIX_IMPORT -DVFI_SEC_DLL_IMPORT

#
# Dependencies
#

SrcFile01  = encryp_aes

AppObjects 	= $(ObjDir)\$(SrcFile01).o


# Library Paths
SDKLibs = 	$(VRXSDK)\lib\VxSTL.so \
			$(VRXSDK)\lib\PED.o \
            $(VRXSDK)\lib\VoyNs.o \
			
EOSLibs =	$(EOS)\lib\CEIF.o \
			$(EOS)\lib\elog.o

ADKLibs =   $(ADK)\lib\liblog.so \
			$(ADK)\lib\libvfisysinfo.so \
			
$(OutDir)\$(Tarjet).vsl : Appbanner $(AppObjects)
	@echo LINKING...$(Tarjet).vsl
    @$(TOOLCHAIN)$(COMM_COMPILER) $(linkOptions) $(AppObjects) $(SDKLibs) $(EOSLibs) $(ADKLibs) -o $(OutDir)\$(Tarjet).vsl



###### Compile #######
# ------------------------------------------------------------
Appbanner:
    @echo.
    @echo **----------------------**
    @echo **     Module AES_ENCRYP**
    @echo **----------------------**
    @echo ** Compiling code files **
    @echo **----------------------**
	@echo.
	@cd $(ObjDir)
	@echo $(ObjDir)
# ------------------------------------------------------------

$(ObjDir)\$(SrcFile01).o : $(SrcDir)\$(SrcFile01).cpp
   @echo $(SrcFile01).cpp 
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile01).cpp

clean:
	del $(ObjDir)\*.o
	del $(OutDir)\AdkCapX
	del $(OutDir)\*.so
	del $(OutDir)\*.axf
	del $(OutDir)\*.vsl
	del $(OutDir)\*.vsa
	del $(OutDir)\*.s
	del $(OutDir)\*.o
	del $(OutDir)\*.map
	del $(OutDir)\*.p7s
	del $(OutDir)\*.crt			
			
			
			
			