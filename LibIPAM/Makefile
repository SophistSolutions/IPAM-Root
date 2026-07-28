export TOP_ROOT	:=	$(abspath ../)/
StroikaRoot		:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir			:=	$(TOP_ROOT)LibIPAM/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk



SHELL=/bin/bash

SubDirs:=\
	Common \
	Metadata \

SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) CONFIGURATION=$(CONFIGURATION) --directory $(dir) -s list-objs))

Objs := $(SUBDIR_OBJS)

LibIPAM:=$(TOP_ROOT)Builds/$(CONFIGURATION)/libIPAM$(LIB_SUFFIX)

all:
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building LibIPAM {$(CONFIGURATION)}:"
	@$(StroikaRoot)/Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all; done
	@#TODO DEBUG WHY THIS GENERATES  warning: jobserver unavailable: using -j1.  Add '+' to parent make rule.
	@#MAKEFLAGS= works around the problem, but its probably not safe --LGP 2025-06-05
	@MAKEFLAGS= ${MAKE} --silent $(LibIPAM)


$(LibIPAM):	$(Objs)
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Creating `$(StroikaRoot)Build/Scripts/SubstituteBackVariables $@` ... "
	@rm -f $@
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then \
		$(StroikaRoot)Build/Scripts/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+2)) '$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))';\
	fi
	@$(call DEFAULT_LIBRARY_GEN_LINE,$@,$(Objs))


clean clobber::
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "LibIPAM $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@for s in ${SubDirs}; do $(MAKE) --directory $${s} -s MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@; done


format-code:
	@$(StroikaRoot)Build/Scripts/FormatCode Common *.h *.inl *.cpp
	@for s in ${SubDirs}; do $(StroikaRoot)Build/Scripts/FormatCode $${s} *.h *.inl *.cpp ; done
