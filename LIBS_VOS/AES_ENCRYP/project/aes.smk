#
# Paths
#

VOS2 = 1

vos2_SYSROOT=C:\vde

#VOS = 1
#CARBON = 1
!if defined(VOS2)
compiler = arm-linux-gnueabihf-g++
VOSTOOLCHAIN       = $(vos2_SYSROOT)\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
#VOSSDK             = $(vos2_SYSROOT)\SDKs\vos2\vos2-sdk-winx86-release-30811900\vos2
#VOSSDK             = $(vos2_SYSROOT)\SDKs\vos2\vos2-sdk-winx86-release-30812100\vos2
VOSSDK             =  C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31342700\vos2
#VFIADK				= \vde\ADKs\adk-full-4.4.13-440\vos2
#VFIADK				= \vde\ADKs\adk-full-4.4.15-498\vos2
VFIADK				= C:\vde\ADKs\adk-full-4.7.19-1342\vos2

#VOSSDK             =  C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31343200-A200\vos2
#VFIADK				= C:\vde\ADKs\adk-full-4.7.22.1-1443\vos2
#
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
# VOSSDK             =  \vde\SDKs\vos\Mx9xxSDK-1.2.7-DTK400.410904
#VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-30410900\Mx9xxSDK-1.2.7-DTK400.410802
#VFIADK				= \vde\ADKs\adk-full-4.3.4-369\vos
VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-31340400\Mx9xxSDK-1.2.7-DTK400.340400
VFIADK				= \vde\ADKs\adk-full-4.7.2-743\vos
LinkOptions = --sysroot=$(VOSSDK) -Wl,-rpath=$(VOSSDK)\usr\local\lib\svcmgr
!endif

LibName = libAES_encryp
Project  = ..\

SrcDir 	= ..\src
ObjDir 	= ..\obj
OutDir 	= ..\output
IncDir  = $(Project)\include

HARDCODERESP=-DHRD_COD_RSP
#HARDCODERESP=


Coptions= $(HARDCODERESP)

# All possible includes
SDKIncludes = -I $(VOSSDK)\usr\include -I $(VOSSDK)\usr\local\include  -I $(VOSSDK)\usr\include\libxml2
ADKIncludes = -I $(VFIADK)\include
APPIncludes = -I $(IncDir)

# Paths to Libraries
SDKLibPaths = -L$(VOSSDK)\lib -L$(VOSSDK)\usr\lib -L$(VOSSDK)\usr\local\lib -L$(VOSSDK)\usr\local\lib\svcmgr 
ADKLibPaths = -L$(VFIADK)\lib

SharedLibs = -lvfiguiprt -lsvcmgr -lvfibuzzer -lasound -lpthread -llog\
   -lm -lz -liniparser -lcjson -lvfiplatforminfo -lstdc++ -lsvcmgrSvcInfXml\
   -lsvc_logmgr -lsvcmgrSvcInf -ldl -lcom -lexpat

#
# Dependencies
#

AppObjects 	= $(ObjDir)\AES_encryp.o

# Link Options
# LinkOptions = -Wl,-unresolved-symbols=ignore-in-shared-libs
#LinkOptions = --sysroot=$(VOSSDK) -Wl
#LinkOptions =

#
#  Link object files
#
$(OutDir)\AES_encryp : $(AppObjects)
	@echo Linking Files
	@$(VOSTOOLCHAIN)\$(compiler) -shared -Wl,-soname,$(LibName).so -o $(OutDir)\$(LibName).so  $(AppObjects) $(LinkOptions) $(SDKLibPaths) $(ADKLibPaths) $(SharedLibs)

#
#  Compile modules
#
$(ObjDir)\AES_encryp.o : $(SrcDir)\encryp_aes.cpp 
	@echo Compliling encryp_aes.cpp
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC -c $(Coptions) $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\AES_encryp.o  $(SrcDir)\encryp_aes.cpp

	
clean:
	del $(ObjDir)\*.o
	del $(OutDir)\AES_encryp
	del $(OutDir)\*.so
	