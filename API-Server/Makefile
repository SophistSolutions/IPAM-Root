export TOP_ROOT=$(abspath ../)/
StroikaRoot	:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)API-Server/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk


include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk

SHELL=/bin/bash


all:
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --no-print-directory all


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "API-Server $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory=Sources --silent $@


format-code:
	@$(StroikaRoot)ScriptsLib/FormatCode Sources *.h *.inl *.cpp
