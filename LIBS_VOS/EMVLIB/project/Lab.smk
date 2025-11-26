#
# Paths
#

VOS2 = 1
#VOS = 1
#CARBON = 1
!if defined(VOS2)
compiler = arm-linux-gnueabihf-g++
VOSTOOLCHAIN       = \vde\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
#VOSSDK             =  \vde\SDKs\vos2\vos2-sdk-winx86-release-30811900\vos2
VOSSDK             =  C:\vde\SDKs\vos2\vos2-sdk-winx86-release-31342700\vos2
#VFIADK				= \vde\ADKs\adk-full-4.4.13-440\vos2
#VFIADK				= \vde\ADKs\adk-full-4.4.15-498\vos2
VFIADK				= C:\vde\ADKs\adk-full-4.7.19-1342\vos2

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
# VOSSDK             =  \vde\SDKs\vos\Mx9xxSDK-1.2.7-DTK400.410904
#VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-30410900\Mx9xxSDK-1.2.7-DTK400.410802
#VFIADK				= \vde\ADKs\adk-full-4.3.4-369\vos
VOSSDK             =  \vde\SDKs\vos\vos-sdk-winx86-release-31340400\Mx9xxSDK-1.2.7-DTK400.340400
VFIADK				= \vde\ADKs\adk-full-4.7.2-743\vos
LinkOptions = --sysroot=$(VOSSDK) -Wl,-rpath=$(VOSSDK)\usr\local\lib\svcmgr
!endif

#Defines
#XMLS= 
XMLS=-DXMLFILES

LibName = libVosEMVProcess


#Project = \Engage_Proy\LIBS_ENGAGE\EMVLIB
#Project = \ADC\FrameworkAlexMLast\EMVLIB
Project = ..\

SrcDir 	= ..\src
ObjDir 	= ..\obj
OutDir 	= ..\output
IncDir  = $(Project)\include


# All possible includes
SDKIncludes = -I $(VOSSDK)\usr\include -I $(VOSSDK)\usr\local\include  -I $(VOSSDK)\usr\include\libxml2
ADKIncludes = -I $(VFIADK)\include
APPIncludes = -I $(IncDir)

# Paths to Libraries
SDKLibPaths = -L$(VOSSDK)\lib -L$(VOSSDK)\usr\lib -L$(VOSSDK)\usr\local\lib -L$(VOSSDK)\usr\local\lib\svcmgr 
ADKLibPaths = -L$(VFIADK)\lib

SharedLibs = -lvfiguiprt -lTLV_Util -lsvcmgr -lvfibuzzer -lasound -lpthread -llog\
   -lm -lz -liniparser -lcjson -lvfiplatforminfo -lstdc++ -lsvcmgrSvcInfXml\
   -lsvc_logmgr -lber-tlv -lsvcmgrSvcInf -ldl -lcom

#
# Dependencies
#
AppObjects 	= $(ObjDir)\VosEMVProcess.o

#Compile Options
COptions = -c -Wno-write-strings $(XMLS)

# Link Options
# LinkOptions = -Wl,-unresolved-symbols=ignore-in-shared-libs
# LinkOptions = 

#
#  Link object files
#
$(OutDir)\VosEMVProcess : $(AppObjects)
	@echo Linking files...
	@$(VOSTOOLCHAIN)\$(compiler) -shared -Wl,-soname,$(LibName).so -o $(OutDir)\$(LibName).so  $(AppObjects) $(LinkOptions) $(SDKLibPaths) $(ADKLibPaths) $(SharedLibs)

#
#  Compile modules
#
$(ObjDir)\VosEMVProcess.o : $(SrcDir)\VosEMVProcess.cpp 
	@echo Compilying VosEMVProcess.cpp 
	@$(VOSTOOLCHAIN)\$(compiler) -fPIC $(COptions) -c $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\VosEMVProcess.o  $(SrcDir)\VosEMVProcess.cpp

clean:
	del $(ObjDir)\*.o
	del $(OutDir)\$(LibName)
	del $(OutDir)\*.so
	