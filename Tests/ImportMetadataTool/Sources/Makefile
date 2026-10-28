export TOP_ROOT=$(abspath ../../../)/
StroikaRoot	:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)Tests/ImportMetadataTool/Sources/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tests/ImportMetadataTool/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tests/ImportMetadataTool/)"


CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT))"
LDFLAGS=

# exiv2.pc names no Windows system libraries, but exiv2's version.cpp calls EnumProcessModules
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(BuildPlatform)))
LDFLAGS += psapi.lib
endif

TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/IPAM/Tests/ImportMetadataTool/
TARGETEXE				=	$(TARGETEXEDir)ImportMetadataTool${EXE_SUFFIX}

SubDirs=\



SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))


Objs	=	\
	$(ObjDir)Main${OBJ_SUFFIX}\

VERSION_FILE_IN=$(TOP_ROOT)VERSION
VERSION_FILE_OUT=$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tests/ImportMetadataTool/AppVersion.h
$(VERSION_FILE_OUT):	$(VERSION_FILE_IN)
	@mkdir -p $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tests/ImportMetadataTool
	@$(StroikaRoot)Build/Scripts/MakeVersionFile $(VERSION_FILE_IN) $(VERSION_FILE_OUT) AppCPlusPlusVersion kAppVersion

$(ObjDir)Main${OBJ_SUFFIX} $(ObjDir)Service${OBJ_SUFFIX}:		$(VERSION_FILE_OUT)


all:
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building IPAM/Tests/ImportMetadataTool"
	@mkdir -p $(TARGETEXEDir)
	@$(MAKE) --silent $(VERSION_FILE_OUT) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))


Objs += $(SUBDIR_OBJS)

$(TARGETEXE):	$(Objs) $(StroikaLibs) $(TOP_ROOT)Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX)
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Linking `$(StroikaRoot)Build/Scripts/SubstituteBackVariables $@`" "... "
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)Build/Scripts/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) '$(call DEFAULT_LINK_LINE,$@,libIPAM exiv2,,${LDFLAGS})';\
	fi
	@$(call DEFAULT_LINK_LINE,$@,libIPAM exiv2,,${LDFLAGS})


clean::
	@rm -f $(SUBDIR_OBJS)
