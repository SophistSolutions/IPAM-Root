export TOP_ROOT:=$(shell realpath ../)/
export StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

Tests	:=	Geolocation ImportMetadataTool Metadata

all clean clobber format-code:
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Tests $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@for t in $(Tests); do\
		$(MAKE) --directory=$$t --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@ || exit $$?;\
	done

# runs every test, even after one fails, and fails if any did
run-tests:
	@$(StroikaRoot)Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@$(StroikaRoot)Build/Scripts/PrintProgressLine $(MAKE_INDENT_LEVEL) "Tests Run {$(CONFIGURATION)}:"
	@failed=0;\
	for t in $(Tests); do\
		$(TOP_ROOT)Builds/$(CONFIGURATION)/IPAM/Tests/$$t/$$t$(EXE_SUFFIX) || failed=1;\
	done;\
	exit $$failed
