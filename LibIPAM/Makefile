export TOP_ROOT	:=	$(abspath ../)/
StroikaRoot		:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir			:=	$(TOP_ROOT)LibIPAM/

include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk



SHELL=/bin/bash

SubDirs:=\
	Common \
	Metadata \

SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))

Objs := $(SUBDIR_OBJS)

LibIPAM:=$(TOP_ROOT)Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX)

all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building LibIPAM {$(CONFIGURATION)}:"
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@#TODO DEBUG WHY THIS GENERATES  warning: jobserver unavailable: using -j1.  Add '+' to parent make rule.
	@#MAKEFLAGS= works around the problem, but its probably not safe --LGP 2025-06-05
	@MAKEFLAGS= ${MAKE} --silent $(LibIPAM)


$(LibIPAM):	$(Objs)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Creating `$(StroikaRoot)ScriptsLib/SubstituteBackVariables $@` ... "
	@rm -f $@
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then \
		$(StroikaRoot)ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+2)) '$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))';\
	fi
	@$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "LibIPAM $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@; done


format-code:
	@$(StroikaRoot)ScriptsLib/FormatCode Common *.h *.inl *.cpp
	@for s in ${SubDirs}; do $(StroikaRoot)ScriptsLib/FormatCode $${s} *.h *.inl *.cpp ; done
