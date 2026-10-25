export TOP_ROOT:=$(shell realpath ../)/
export StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

all clean clobber format-code:
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Tests $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory=Geolocation --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@

run-tests:
	@$(StroikaRoot)Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Tests Run {$(CONFIGURATION)}:"
	@$(TOP_ROOT)Builds/$(CONFIGURATION)/IPAM/Tests/Geolocation/Geolocation$(EXE_SUFFIX)