export TOP_ROOT	:=	$(abspath ../../../)/
StroikaRoot		:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir			:=	$(TOP_ROOT)IPAM/Tools/PS2/Sources/
ObjDir			:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS2/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS2/)"

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT))"
LDFLAGS=
LDFLAGS += $(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)/Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX))


TARGETEXEDir	:=	$(StroikaRoot)Builds/$(CONFIGURATION)/IPAM/Tools/PS2/
TARGETEXE		:=	$(TARGETEXEDir)PS2${EXE_SUFFIX}


SubDirs:=\



SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))


Objs	:=	\
	$(ObjDir)Main${OBJ_SUFFIX}\


VERSION_FILE_IN=$(TOP_ROOT)VERSION
VERSION_FILE_OUT=$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS2/AppVersion.h
$(VERSION_FILE_OUT):	$(VERSION_FILE_IN)
	@mkdir -p $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS2
	@$(StroikaRoot)Build/Scripts/MakeVersionFile $(VERSION_FILE_IN) $(VERSION_FILE_OUT) AppCPlusPlusVersion kAppVersion __PS2_AppVersion_H__

$(ObjDir)Main${OBJ_SUFFIX} $(ObjDir)Service${OBJ_SUFFIX}:		$(VERSION_FILE_OUT)


all:
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building IPAM/Tools/PS2"
	@mkdir -p $(TARGETEXEDir)
	@$(MAKE) --silent $(VERSION_FILE_OUT) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))


Objs += $(SUBDIR_OBJS)

$(TARGETEXE):	$(Objs) $(StroikaLibs)
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Linking `$(StroikaRoot)Build/Scripts/SubstituteBackVariables $@`" "... "
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)Build/Scripts/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) '$(call DEFAULT_LINK_LINE,$@,stroika-frameworks,,${LDFLAGS})';\
	fi
	@$(call DEFAULT_LINK_LINE,$@,stroika-frameworks,,${LDFLAGS})


clean::
	@rm -f $(SUBDIR_OBJS)
