SOURCE_DIR=src
OBJ_DIR=obj
OUT_DIR=out
TARGET_FILE=$(APP_NAME)

#CORE_LIB= 	corvet.a
RDC_Libs =  -lCmdPP -lComRdc -lAdkCapX -lVosEMVProcess -lAES_encryp
ADK_LIBS= 	-lsvc_led \
			-lvfisysinfo \
			-lvfiguiprt \
			-lvfibuzzer \
			-lsvc_powermngt \
			-llog \
			-lrt \
			-lm \
			-lvfiplatforminfo \
			-lstdc++ \
			-ldl \
			-lcom \
			-lvfiipc \
			-linf \
			-lsqlite \
			-lexpat \
			-lvfisvc \
			-lvfisec \
			-ltec \
			-lEMV_CT_Framework \
			-lEMV_CT_Link \
			-lEMV_CT_Client \
			-lEMV_CTLS_Link \
			-lEMV_CTLS_Framework \
			-lEMV_CTLS_Client \
			-lTLV_Util \
			-lmsr \
			-lsvc_sound \
			-lsvc_tms \
			-levt \
			-lvfisysbar \
			$(RDC_Libs)	

#-lasound 
#-lz \
#-lcjson \

ADK_INCLUDES=  -I$(ADK_PATH)\include
APP_INCLUDES=  -IC:\Proyectos\LDI\adc-lac-mex-engage-elavon-calimax\inc
SDK_INCLUDES = -I$(SDK_PATH)\usr\local\include -I$(SDK_PATH)\usr\include
USRLibPaths =   C:\Proyectos\LDI\adc-lac-mex-engage-elavon-calimax\lib
			
SOURCES := $(wildcard ../SRC/*.cpp) $(wildcard ../SRC/**/*.cpp) 
APP_OBJECTS := $(patsubst %.cpp, %.o, ${SOURCES})
APP_OBJECTSX := $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp, %.o, $(notdir ${SOURCES})))

HARDCODERESP=-DHRD_COD_RSP
#HARDCODERESP=

POLARBIGNUMC=-DPOLARSSL_BIGNUM_C
RSA=-DPOLARSSL_RSA_C
DUKPT=-DMAKE_DUKPT
ARM=-D__arm



DIRECTIVAS = $(HARDCODERESP) $(POLARBIGNUMC) $(RSA) $(DUKPT) $(ARM)

$(OUT_DIR)\$(TARGET_FILE) : $(APP_OBJECTS)
	@echo  Linking...

#	@$(LINKER) -o $(OUT_DIR)\$(TARGET_FILE) $(LOPTIONS) $(APP_OBJECTSX) $(CORE_LIB_PATH)\$(CORE_LIB) -L$(USRLibPaths) -L$(SDK_LIB_PATH) -L$(SDK_PATH)\lib -L$(SDK_LIB_PATH)\svcmgr $(SDK_LIBS) -L$(ADK_LIB_PATH) $(ADK_LIBS) -L$(CORE_LIB_PATH)
	@$(LINKER) -o $(OUT_DIR)\$(TARGET_FILE) $(LOPTIONS) $(APP_OBJECTSX) -L$(USRLibPaths) -L$(SDK_LIB_PATH) -L$(SDK_PATH)\lib  -L$(SDK_LIB_PATH)\svcmgr $(SDK_LIBS) -L$(ADK_LIB_PATH) $(ADK_LIBS)

# $(APP_OBJECTS) : %.o : %.cpp : pch.h.gch
$(APP_OBJECTS) : %.o : %.cpp
	@echo  Compiling $<
	@$(COMPILER) $(COPTIONS) $(DIRECTIVAS) $(SDK_INCLUDES) $(ADK_INCLUDES) $(APP_INCLUDES) -o $(OBJ_DIR)/$(notdir $@) $<
		 
# PCH is built just like all other source files
#pch.h.gch: 
#	@echo Compiling precompiled header
#	@$(COMPILER) $(COPTIONS) $(DIRECTIVAS) $(ADK_INCLUDES) $(APP_INCLUDES)  $(SOURCE_DIR)/pch.h -o $(SOURCE_DIR)/pch.h.gch 

