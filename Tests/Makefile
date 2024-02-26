export TOP_ROOT:=$(shell realpath ../)/
export StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk

all clean clobber format-code:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Tests $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory=Geolocation --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@

## tmphack - fix to do configuration stuff
run-tests:
	../Builds/Debug/IPAM/Tests/Geolocation/Geolocation.exe