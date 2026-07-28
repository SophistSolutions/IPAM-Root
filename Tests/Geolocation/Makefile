export TOP_ROOT:=$(abspath ../../)/
StroikaRoot	:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)Tests/Geolocation/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk



SHELL=/bin/bash


all:
	@$(StroikaRoot)/Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --no-print-directory all


clean clobber::
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "IPAM/Tests/Geolocation $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(StroikaRoot)/Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --silent $@


format-code:
	@$(StroikaRoot)Build/Scripts/FormatCode Sources *.h *.inl *.cpp
