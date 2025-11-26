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

Tarjet 		= libVrxEMVProcess

#Project = C:\Engage_Proy\LIBS_ENGAGE\BNMX\EMVLIB
Project = C:\BnteBnrgoPPVOS2_EVO\PINpadVOS2\LIBS_VOS\EMVLIB


SrcDir 	= $(Project)\src
ObjDir 	= $(Project)\obj
OutDir 	= $(Project)\output
IncDir  = $(Project)\include
ModDir	= $(Modules)\Utilities\include
ModDir1	= $(Modules)\UserInterface\include

# Include Paths
SDKIncludes		= -I$(VRXSDK)\include
ADKIncludes		= -I$(ADK)\include
EOSIncludes  	= -I$(EOS)\include
APPIncludes		= -I$(IncDir)
MODIncludes		= -I$(ModDir) -I$(ModDir1)

XMLS = -DXMLFILES
SYMBOLS_APP = $(XMLS)

# for release version change the COptions to 
COptions = -vsolib -c -p -g -O2 -armcc,--exceptions -armcc,--diag-suppress=1  -armcc,--diag-suppress=1300 -armcc,--diag-suppress=1299 -armcc,--diag-suppress=177  -DUTIL_IMPORT -DUSERINTERFACE_IMPORT -DLOGAPI_ENABLE_DEBUG -D_DEBUG -DVFI_IPC_DLL_IMPORT -DVFI_SYSINFO_DLL_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_MAC_DLL_IMPORT -D_VRXEVO -DVFI_COM_DLL_IMPORT -D_EXCEPTION_LOGGING -DLIB_CONFIG -DVFI_GUIPRT_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_POSIX_IMPORT -DVFI_SEC_DLL_IMPORT $(SYMBOLS_APP)

#
# Dependencies
#

SrcFile01  = VosEMVProcess

AppObjects 	= $(ObjDir)\$(SrcFile01).o


# Library Paths
SDKLibs = 	$(VRXSDK)\lib\VxSTL.so \
			$(VRXSDK)\lib\PED.o \
            $(VRXSDK)\lib\VoyNs.o \
			
EOSLibs =	$(EOS)\lib\CEIF.o \

ADKLibs =   $(ADK)\lib\libposix.a \
			$(ADK)\lib\liblog.so \
			$(ADK)\lib\libvfiguiprt.so \
			$(ADK)\lib\libvfiipc.a \
			$(ADK)\lib\libTLV_Util.a \
			$(ADK)\lib\libmsr.so \
			$(ADK)\lib\libevt.so \
            $(ADK)\lib\libEMV_CT_Client.so \
			$(ADK)\lib\libEMV_CTLS_Client.so \
			$(ADK)\lib\libsec.so \
			$(ADK)\lib\libvfisysinfo.so \
			$(ADK)\lib\libtec.so \
			$(ADK)\lib\libvfisysbar.so \
			$(ADK)\lib\libtms.a \
			$(ADK)\lib\libevt-static.a \

CURLLibs = 	$(CURL)\lib\libcurl.o \
			
			
	
$(OutDir)\$(Tarjet).vsl : Appbanner $(AppObjects)
	@echo LINKING...$(Tarjet).vsl
    $(TOOLCHAIN)$(COMM_COMPILER) $(linkOptions) $(AppObjects) $(SDKLibs) $(EOSLibs) $(ADKLibs) -o $(OutDir)\$(Tarjet).vsl 


	
###### Compile #######
# ------------------------------------------------------------
Appbanner:
    @echo.
    @echo **----------------------**
    @echo ** Module  Process  EMV **
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
	del $(OutDir)\ProcessEMV
	del $(OutDir)\*.so
	del $(OutDir)\*.axf
	del $(OutDir)\*.vsl
	del $(OutDir)\*.vsa
	del $(OutDir)\*.s
	del $(OutDir)\*.o
	del $(OutDir)\*.map
	del $(OutDir)\*.p7s
	del $(OutDir)\*.crt	
