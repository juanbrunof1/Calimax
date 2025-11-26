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

Tarjet 		= libAdkCapX

#Project = C:\Engage_Proy\LIBS_ENGAGE\BNMX\ADK_CAPX
Project = C:\BnteBnrgoPPVOS2_EVO\PINpadVOS2\LIBS_VOS\ADK_CAPX

SrcDir 	= $(Project)\src
ObjDir 	= $(Project)\obj
OutDir 	= $(Project)\output
IncDir  = $(Project)\inc

# Include Paths
SDKIncludes		= -I$(VRXSDK)\include
ADKIncludes		= -I$(ADK)\include
EOSIncludes  	= -I$(EOS)\include
APPIncludes		= -I$(IncDir)
# MODIncludes		= -I$(ModDir)


HARDCODERESP=-DHRD_COD_RSP
#HARDCODERESP=
POLARBIGNUMC=-DPOLARSSL_BIGNUM_C
RSA=-DPOLARSSL_RSA_C
ARM=-D__arm
DUKPT=-DMAKE_DUKPT

SYMBOLS_APP = $(HARDCODERESP) $(ARM) $(DUKPT) $(EVO_CORE)


# for release version change the COptions to 
COptions = -vsolib -c -p -g -O2 -armcc,--exceptions -armcc,--diag-suppress=1  -armcc,--diag-suppress=1300 -armcc,--diag-suppress=1299 -armcc,--diag-suppress=177 $(SYMBOLS_APP) -DLOGAPI_ENABLE_DEBUG -D_DEBUG -DVFI_IPC_DLL_IMPORT -DVFI_SYSINFO_DLL_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_MAC_DLL_IMPORT -D_VRXEVO -DVFI_COM_DLL_IMPORT -D_EXCEPTION_LOGGING -DLIB_CONFIG -DVFI_GUIPRT_IMPORT -DVFI_SYSBAR_DLL_IMPORT -DVFI_POSIX_IMPORT -DVFI_SEC_DLL_IMPORT

#
# Dependencies
#

SrcFile01  = bignum
SrcFile02  = crpdkp
SrcFile03  = dkprsa
SrcFile04  = usrcmds
SrcFile05  = dukpt_defs
SrcFile06  = rsa
SrcFile07  = file
SrcFile08  = GetSysInfo
# SrcFile09  = comm
SrcFile10  = debug



AppObjects 	= $(ObjDir)\$(SrcFile01).o \
			  $(ObjDir)\$(SrcFile02).o \
			  $(ObjDir)\$(SrcFile03).o \
			  $(ObjDir)\$(SrcFile04).o \
			  $(ObjDir)\$(SrcFile05).o \
			  $(ObjDir)\$(SrcFile06).o \
              $(ObjDir)\$(SrcFile07).o \
			  $(ObjDir)\$(SrcFile08).o \
#			  $(ObjDir)\$(SrcFile09).o \
			  $(ObjDir)\$(SrcFile10).o


# Library Paths
SDKLibs = 	$(VRXSDK)\lib\VxSTL.so \
			$(VRXSDK)\lib\PED.o \
            $(VRXSDK)\lib\VoyNs.o \
			
EOSLibs =	$(EOS)\lib\CEIF.o \
			$(EOS)\lib\elog.o

ADKLibs =   $(ADK)\lib\liblog.so \
			$(ADK)\lib\libvfisysinfo.so \

# MODLibs = 	$(Project)\lib\libUtilities.so

$(OutDir)\$(Tarjet).vsl : Appbanner $(AppObjects)
	@echo LINKING...$(Tarjet).vsl
    @$(TOOLCHAIN)$(COMM_COMPILER) $(linkOptions) $(AppObjects) $(SDKLibs) $(EOSLibs) $(ADKLibs) $(MODLibs) -o $(OutDir)\$(Tarjet).vsl 			


	
###### Compile #######
# ------------------------------------------------------------
Appbanner:
    @echo.
    @echo **----------------------**
    @echo **     Module  CAPX     **
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

$(ObjDir)\$(SrcFile02).o : $(SrcDir)\$(SrcFile02).cpp
   @echo $(SrcFile02).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile02).cpp

$(ObjDir)\$(SrcFile03).o : $(SrcDir)\$(SrcFile03).cpp
   @echo $(SrcFile03).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile03).cpp

$(ObjDir)\$(SrcFile04).o : $(SrcDir)\$(SrcFile04).cpp
   @echo $(SrcFile04).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile04).cpp
   
$(ObjDir)\$(SrcFile05).o : $(SrcDir)\$(SrcFile05).cpp
   @echo $(SrcFile05).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile05).cpp

$(ObjDir)\$(SrcFile06).o : $(SrcDir)\$(SrcFile06).cpp
   @echo $(SrcFile06).c
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\$(SrcFile06).cpp

$(ObjDir)\$(SrcFile07).o : $(SrcDir)\logs\$(SrcFile07).cpp
  @echo $(SrcFile07).cpp
  @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\logs\$(SrcFile07).cpp
 
$(ObjDir)\$(SrcFile08).o : $(SrcDir)\sysinfo\$(SrcFile08).cpp
   @echo $(SrcFile08).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\sysinfo\$(SrcFile08).cpp       

# $(ObjDir)\$(SrcFile09).o : $(SrcDir)\logs\$(SrcFile09).cpp
#   @echo $(SrcFile09).cpp
#   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\logs\$(SrcFile09).cpp       

$(ObjDir)\$(SrcFile10).o : $(SrcDir)\logs\$(SrcFile10).cpp
   @echo $(SrcFile10).cpp
   @$(TOOLCHAIN)$(COMM_COMPILER) $(SDKIncludes) $(ADKIncludes) $(EOSIncludes) $(APPIncludes) $(MODIncludes) $(COptions) $(SrcDir)\logs\$(SrcFile10).cpp       

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
