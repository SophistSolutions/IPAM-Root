export TOP_ROOT=$(abspath ../../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)Tools/PS1/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS1/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS1/)"


CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT))"
LIB_DEPENDENCIES += $(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)/Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX))


ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
LIB_DEPENDENCIES += exiv2.lib
LIB_DEPENDENCIES += libexpat.lib psapi.lib
else
LIB_DEPENDENCIES += -lexiv2 -lexpat -lz
endif

# From https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4286?view=msvc-170
#		Sounds like fix needed in exiv2.lib
# LINK : warning LNK4286: symbol 'XML_Parse' defined in 'libexpat.lib(xmlparse.obj)' is imported by 'exiv2.lib(xmp.obj)'
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
EXTRA_SUFFIX_LINKER_ARGS+= -ignore:4286 
endif

TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/IPAM/Tools/PS1/
TARGETEXE				=	$(TARGETEXEDir)PS1${EXE_SUFFIX}


SubDirs=\



SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))


Objs	=	\
	$(ObjDir)Main${OBJ_SUFFIX}\
	$(ObjDir)Digikam${OBJ_SUFFIX}\
	$(ObjDir)DocumentMetadata${OBJ_SUFFIX}\
	$(ObjDir)ImageMetadataExtraction${OBJ_SUFFIX}\

VERSION_FILE_IN=$(TOP_ROOT)VERSION
VERSION_FILE_OUT=$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS1/AppVersion.h
$(VERSION_FILE_OUT):	$(VERSION_FILE_IN)
	@mkdir -p $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/IPAM/Tools/PS1
	@$(StroikaRoot)ScriptsLib/MakeVersionFile $(VERSION_FILE_IN) $(VERSION_FILE_OUT) AppCPlusPlusVersion kAppVersion __PS1_AppVersion_H__

$(ObjDir)Main${OBJ_SUFFIX} $(ObjDir)Service${OBJ_SUFFIX}:		$(VERSION_FILE_OUT)


all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building IPAM/Tools/PS1"
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
