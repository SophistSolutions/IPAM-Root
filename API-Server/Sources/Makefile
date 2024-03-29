export TOP_ROOT=$(abspath ../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)API-Server/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/API-Server/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/API-Server/)"

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT))"
LinkerArgs_LibDependencies += $(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)/Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX))



TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/IPAM/API-Server/
TARGETEXE				=	$(TARGETEXEDir)API-Server${EXE_SUFFIX}


SubDirs=\



SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))


Objs	=	\
	$(ObjDir)Main${OBJ_SUFFIX}\


VERSION_FILE_IN=$(TOP_ROOT)VERSION
VERSION_FILE_OUT=$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/API-Server/AppVersion.h
$(VERSION_FILE_OUT):	$(VERSION_FILE_IN)
	@mkdir -p $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/API-Server
	@$(StroikaRoot)ScriptsLib/MakeVersionFile $(VERSION_FILE_IN) $(VERSION_FILE_OUT) AppCPlusPlusVersion kAppVersion __IPAM_AppVersion_H__

$(ObjDir)Main${OBJ_SUFFIX} $(ObjDir)Service${OBJ_SUFFIX}:		$(VERSION_FILE_OUT)


all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building API-Server"
	@mkdir -p $(TARGETEXEDir)
	@$(MAKE) --silent $(VERSION_FILE_OUT) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))


Objs += $(SUBDIR_OBJS)

$(TARGETEXE):	$(Objs) $(StroikaLibs)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Linking `$(StroikaRoot)ScriptsLib/SubstituteBackVariables $@`" "... "
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) '$(call DEFAULT_LINK_LINE,$@)';\
	fi
	@$(call DEFAULT_LINK_LINE,$@)


clean::
	@rm -f $(SUBDIR_OBJS)
