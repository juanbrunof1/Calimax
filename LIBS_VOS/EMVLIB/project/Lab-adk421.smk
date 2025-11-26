#
# Paths
#
VOS2TOOLCHAIN       = \vde\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
#VOS2SDK            = \vde\SDKs\vos2\vos2-sdk-winx86-release-30380200
VOS2SDK             = \vde\SDKs\vos2\vos2-sdk-winx86-release-30410201\vos2
#VFIADK				= \vde\ADKs\adk-full-ext-3.1.1-165\vos2
#VFIADK				= \vde\ADKs\adk-full-4.1.0-RC2-62\vos2
VFIADK				= \vde\ADKs\adk-full-ext-4.1.0-94\vos2

VOS2TOOLCHAIN		= \vde\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
VOS2SDK        		= \toolchains\vos2-sdk-winx86-release-30410201\vos2
VFIADK				= \VDE\ADKs\adk-full-4.1.0-94\vos2

VOS2TOOLCHAIN		= \vde\toolchains\windows\vos2\gcc-linaro-arm-linux-gnueabihf-4.7-2013.03\bin
VOS2SDK         	= \toolchains\vos2-sdk-winx86-release-30410400\vos2
VFIADK				= \VDE\ADKs\adk-full-4.2.0-138\vos2


#Defines
#XMLS= 
XMLS=-DXMLFILES

LibName = libVosEMVProcess

Project = ..\
Project = ..\

SrcDir 	= ..\src
ObjDir 	= ..\obj
OutDir 	= ..\output
IncDir  = $(Project)\include


# All possible includes
SDKIncludes = -I $(VOS2SDK)\usr\include -I $(VOS2SDK)\usr\local\include  -I $(VOS2SDK)\usr\include\libxml2
ADKIncludes = -I $(VFIADK)\include
APPIncludes = -I $(IncDir)

# Paths to Libraries
SDKLibPaths = -L$(VOS2SDK)\lib -L$(VOS2SDK)\usr\lib -L$(VOS2SDK)\usr\local\lib -L$(VOS2SDK)\usr\local\lib\svcmgr 
ADKLibPaths = -L$(VFIADK)\lib

SharedLibs = -lvfiguiprt -lEMV_CT_Framework -lEMV_CT_Client -lEMV_CTLS_Framework -lEMV_CTLS_Client -lTLV_Util -lvfictls -lsvcmgr -lvfibuzzer -lasound -lpthread -llog\
   -lm -lz -liniparser -lcjson -lvfiplatforminfo -lstdc++ -lsvcmgrSvcInfXml\
   -lsvc_logmgr -llogapi -lber-tlv -lsvcmgrSvcInf -ldl -lcom

#
# Dependencies
#
AppObjects 	= $(ObjDir)\VosEMVProcess.o

#Compile Options
COptions = -c -Wno-write-strings $(XMLS)

# Link Options
# LinkOptions = -Wl,-unresolved-symbols=ignore-in-shared-libs
LinkOptions = 

#
#  Link object files
#
$(OutDir)\VosEMVProcess : $(AppObjects)
	@echo Linking files...
	@$(VOS2TOOLCHAIN)\arm-linux-gnueabihf-g++ -shared -Wl,-soname,$(LibName).so -o $(OutDir)\$(LibName).so  $(AppObjects) $(LinkOptions) $(SDKLibPaths) $(ADKLibPaths) $(SharedLibs)

#
#  Compile modules
#
$(ObjDir)\VosEMVProcess.o : $(SrcDir)\VosEMVProcess.cpp 
	@echo Compilying VosEMVProcess.cpp 
	@$(VOS2TOOLCHAIN)\arm-linux-gnueabihf-g++ -fPIC $(COptions) -c $(SDKIncludes) $(ADKIncludes) $(APPIncludes) -o $(ObjDir)\VosEMVProcess.o  $(SrcDir)\VosEMVProcess.cpp

clean:
	del $(ObjDir)\*.o
	del $(OutDir)\$(LibName)
	del $(OutDir)\*.so
	