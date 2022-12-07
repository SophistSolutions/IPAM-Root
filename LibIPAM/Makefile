export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

SrcDir				=	$(TOP_ROOT)LibIPAM/


SHELL=/bin/bash

SubDirs=\
	Common

SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))

Objs = $(SUBDIR_OBJS)

ifeq (Unix,$(ProjectPlatformSubdir))
LibIPAM=$(TOP_ROOT)Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX)
else
LibIPAM=$(TOP_ROOT)Builds/$(CONFIGURATION)/LibIPAM$(LIB_SUFFIX)
endif

all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building LibIPAM {$(CONFIGURATION)}:"
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@$(MAKE) -s $(LibIPAM)


$(LibIPAM):	$(Objs)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Creating `$(StroikaRoot)ScriptsLib/SubstituteBackVariables $@` ... "
	@rm -f $@
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then \
		$(StroikaRoot)ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+2)) '$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))';\
	fi
	@$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "LibIPAM $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@; done


format-code:
	@$(StroikaRoot)ScriptsLib/FormatCode Common *.h *.inl *.cpp
