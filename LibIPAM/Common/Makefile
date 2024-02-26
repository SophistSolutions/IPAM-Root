export TOP_ROOT:=$(abspath ../../)/
StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir		:=	$(TOP_ROOT)LibIPAM/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/LibIPAM/Common/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

# CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/API-Server/)"



SubDirs:=\


Objs	:=	\
	$(ObjDir)Geolocation${OBJ_SUFFIX}\


all:	
	mkdir -p $(ObjDir)
	$(MAKE) -s $(Objs)


Objs += $(SUBDIR_OBJS)


clean::
	@rm -f $(SUBDIR_OBJS)
