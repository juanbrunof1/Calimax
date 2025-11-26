#
# Paths
#

VOS2 = 1
#VOS = 1
#CARBON = 1
!if defined(VOS2)
compiler = arm-linux-gnueabihf-g++
VOSTOOLCHAIN       = c:\vde\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
#VOSSDK             =  \vde\SDKs\vos2\vos2-sdk-winx86-release-30811900\vos2
#VOSSDK             =  \vde\SDKs\vos2\vos2-sdk-winx86-release-30812100\vos2
#VOSSDK             =  C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31342700\vos2


#VFIADK				= \vde\ADKs\adk-full-4.4.13-440\vos2
#VFIADK				= \vde\ADKs\adk-full-4.4.15-498\vos2
#VFIADK				= C:\vde\ADKs\adk-full-4.7.19-1342\vos2

#VOSSDK             =  C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31343200-A200\vos2
#VFIADK				= C:\vde\ADKs\adk-full-4.7.22.1-1443\vos2


VOSSDK            = C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31343300-A200\vos2
VFIADK				= C:\vde\ADKs\adk-full-4.7.23.1-1483\vos2

!if defined(CARBON)
VOSSDK             =  \vde\SDKs\vos2\vos2-sdk-winx86-release-30530704\vos2
VFIADK				= \vde\ADKs\adk-full-CARBON_1.0.19-456\vos2
!endif
LinkOptions =
!endif

!if defined(VOS)
compiler = arm-verifone-linux-gnueabi-g++
VOSTOOLCHAIN       = \vde\toolchains\windows\vos\arm-verifone-linux-gnueabi\bin
#VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-30410900\Mx9xxSDK-1.2.7-DTK400.410802
#VFIADK				= \vde\ADKs\adk-full-4.3.4-369\vos
VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-31340400\Mx9xxSDK-1.2.7-DTK400.340400
VFIADK				= \vde\ADKs\adk-full-4.7.2-743\vos
LinkOptions = --sysroot=$(VOSSDK) -Wl,-rpath=$(VOSSDK)\usr\local\lib\svcmgr
!endif


LibName = libAdkCapX

Project = ..\

SrcDir 	= ..\src
ObjDir 	= ..\obj
OutDir 	= ..\output
IncDir  = $(Project)\inc


HARDCODERESP=-DHRD_COD_RSP
#HARDCODERESP=
POLARBIGNUMC=-DPOLARSSL_BIGNUM_C
RSA=-DPOLARSSL_RSA_C
ARM=-D__arm
#DEBUGVOS=-D__VOS_DEBUG
DEBUGVOS=
DUKPT=-DMAKE_DUKPT

Coptions= $(HARDCODERESP) $(POLARBIGNUMC) $(RSA) $(ARM) $(DUKPT) $(DEBUGVOS)

# All possible includes
SDKIncludes = -I $(VOSSDK)\usr\include -I $(VOSSDK)\usr\local\include  -I $(VOSSDK)\usr\include\libxml2
ADKIncludes = -I $(VFIADK)\include -I $(VFIADK)\usr\local\include
APPIncludes = -I $(IncDir)

# Paths to Libraries
SDKLibPaths = -L$(VOSSDK)\lib -L$(VOSSDK)\usr\lib -L$(VOSSDK)\usr\local\lib -L$(VOSSDK)\usr\local\lib\svcmgr 
ADKLibPaths = -L$(VFIADK)\lib

SharedLibs = -lvfiguiprt -lsvcmgr -lvfibuzzer -lasound -lpthread -llog\
   -lm -lz -liniparser -lcjson -lvfiplatforminfo -lstdc++ -lsvcmgrSvcInfXml\
   -lsvc_logmgr -lsvcmgrSvcInf -ldl -lcom

#
# Dependencies
#
AppObjects 	= $(ObjDir)\bignum.o $(ObjDir)\crpdkp.o $(ObjDir)\dkprsa.o $(ObjDir)\usrcmds.o $(ObjDir)\dukpt_defs.o $(ObjDir)\rsa.o \
              $(ObjDir)\comm.o  $(ObjDir)\debug.o  $(ObjDir)\file.o $(ObjDir)\GetSysInfo.o

# Link Options
# LinkOptions = -Wl,-unresolved-symbols=ignore-in-shared-libs
#LinkOptions = --sysroot=$(VOSSDK) -Wl
#LinkOptions =

#
#  Link object files
#
$(OutDir)\AdkCapX : $(AppObjects)
	@echo Linking files...
	@$(VOSTOOLCHAIN)\$(compiler) -shared -Wl,-soname,$(LibName).so -o $(OutDir)\$(LibName).so  $(AppObjects) $(LinkOptions) $(SDKLibPaths) $(ADKLibPaths) $(SharedLibs)

#
#  Compile modules
#

$(ObjDir)\bignum.o : $(SrcDir)\bignum.c
	@echo Compiling bignum.c
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\bignum.o  $(SrcDir)\bignum.c

$(ObjDir)\crpdkp.o : $(SrcDir)\crpdkp.cpp
	@echo Compiling crpdkp.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\crpdkp.o  $(SrcDir)\crpdkp.cpp

$(ObjDir)\dkprsa.o : $(SrcDir)\dkprsa.cpp 
	@echo Compiling dkprsa.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\dkprsa.o  $(SrcDir)\dkprsa.cpp

$(ObjDir)\dukpt_defs.o : $(SrcDir)\dukpt_defs.cpp
	@echo Compiling dukpt_defs.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\dukpt_defs.o  $(SrcDir)\dukpt_defs.cpp

$(ObjDir)\rsa.o : $(SrcDir)\rsa.cpp
	@echo Compiling rsa.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\rsa.o  $(SrcDir)\rsa.cpp

$(ObjDir)\usrcmds.o : $(SrcDir)\usrcmds.cpp
	@echo Compiling usrcmds.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\usrcmds.o  $(SrcDir)\usrcmds.cpp
	
$(ObjDir)\comm.o : $(SrcDir)\logs\comm.cpp
	@echo Compiling comm.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\comm.o  $(SrcDir)\logs\comm.cpp

$(ObjDir)\debug.o : $(SrcDir)\logs\debug.cpp
	@echo Compiling debug.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\debug.o  $(SrcDir)\logs\debug.cpp

$(ObjDir)\file.o : $(SrcDir)\logs\file.cpp
	@echo Compiling file.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\file.o  $(SrcDir)\logs\file.cpp

$(ObjDir)\GetSysInfo.o : $(SrcDir)\sysinfo\GetSysInfo.cpp
	@echo Compiling GetSysInfo.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\GetSysInfo.o  $(SrcDir)\sysinfo\GetSysInfo.cpp
	
clean:
	del $(ObjDir)\*.o
	del $(OutDir)\AdkCapX
	del $(OutDir)\*.so
	