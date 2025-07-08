export TOP_ROOT	:=  $(abspath ../../)/
StroikaRoot		:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir			:=	$(TOP_ROOT)Tools/PS1/

include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk



SHELL=/bin/bash


all:
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --no-print-directory all


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "IPAM/Tools/PS1 $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory=Sources --silent $@


format-code:
	@$(StroikaRoot)ScriptsLib/FormatCode Sources *.h *.inl *.cpp
