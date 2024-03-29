export TOP_ROOT:=$(abspath ./)/
StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifeq (,$(wildcard $(StroikaRoot)Makefile))
$(warning "submodules missing: perhaps you should run `git submodule update --init --recursive`")
endif

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk



#Handy shortcut
CONFIGURATION_TAGS?=$(TAGS)

APPLY_CONFIGS=$(or \
				$(CONFIGURATION), \
				$(if $(CONFIGURATION_TAGS), \
					$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"),\
					$(if $(filter clobber, $(MAKECMDGOALS)),\
						$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --all --quiet),\
						$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --all-default)\
					)\
				)\
			)

all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building IPAM all{$(CONFIGURATION)}:"
	@$(MAKE) -silent IntermediateFiles/ASSURE_DEFAULT_CONFIGURATIONS_BUILT MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@#Cannot use APPLY_CONFIGS here because ConfigurationFiles may have changed and evaluated before here
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)" --all-default` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=LibIPAM --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=Tools --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=Tests --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=API-Server --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
endif


IntermediateFiles/ASSURE_DEFAULT_CONFIGURATIONS_BUILT:
ifeq ($(shell $(StroikaRoot)ScriptsLib/GetConfigurations --quiet),)
	@$(MAKE) -silent default-configurations MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif


STROIKA_CONFIG_PARAMS_COMMON=

# Assume C++20, mostly (maybe fully soon)
ifeq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
STROIKA_CONFIG_PARAMS_COMMON+=	--cppstd-version c++20
endif

# Don't support MFC, we dont use anyhow (speeds builds)
STROIKA_CONFIG_PARAMS_COMMON+=	--ATLMFC no




### @todo FIX - PROBABLY WRONG STRATEGY - FIX ELSEWHERE IN BUILD PROCESS SO ONLY appropriate
### COMPONENTS PULL THIS INTO PATH
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
# STROIKA_CONFIG_PARAMS_COMMON+=	--append-includes-path $(TOP_ROOT)ThirdPartyComponents/exiv2/include/
# STROIKA_CONFIG_PARAMS_COMMON+=	--append-libs-path $(TOP_ROOT)ThirdPartyComponents/exiv2/lib/
# STROIKA_CONFIG_PARAMS_COMMON+=	--append-lib-dependencies exiv2.lib
# STROIKA_CONFIG_PARAMS_COMMON+=	--append-lib-dependencies libexpat.lib
endif


STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
### address sanitizer on Windows produces lots of non-obvious errors (probably false positive).
### anyhow, disable for a little bit until we have time to look into it
### 	- Opened MSFT issue - https://developercommunity.visualstudio.com/t/https:developercommunityvisualstudio/1470855?entry=myfeedback
###		ASAN works, so you can re-enable it occasionally, but it really slows things down alot and pollutes the debug log
###		with TONS of spurrious nonsense, so best to leave disabled.
### 		-- LGP 2021-07-08
STROIKA_CONFIG_PARAMS_DEBUG += --sanitize none
endif



.PHONY: default-configurations
default-configurations:
	@if [ ! -d ConfigurationFiles ] ; then $(MAKE) --silent build-root; fi
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Configuring...
ifeq ($(DETECTED_HOST_OS), Darwin)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --arch x86_64 --build-by-default never --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --arch x86_64 --build-by-default never --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-Logging --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 --trace2file enable $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-x86 --arch x86 --build-by-default never --config-tag Windows --config-tag x86 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-x86_64 --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-x86 --arch x86 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-x86_64 --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --compiler-driver g++-11 --only-if-has-compiler $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --compiler-driver g++-11 --only-if-has-compiler $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
endif
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
	@$(MAKE) --silent Builds/__AUTOMATIC_MAKE_PROJECT_FILES__
endif
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Applying configuration(s) to vscode:"
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --all` ; do\
		$(StroikaRoot)ScriptsLib/ApplyConfiguration --only-vscode $$i;\
	done

build-root:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Making BuildRoot...
	@export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./ScriptsLib/MakeBuildRoot ../../../

apply-configurations-to-vscode:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Applying configuration(s) to vscode:"
	@for i in $(APPLY_CONFIGS) ; do\
		MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $(StroikaRoot)ScriptsLib/ApplyConfiguration --only-vscode $$i;\
	done

list-configurations list-configuration-tags apply-configurations apply-configuration apply-configurations-if-needed reconfigure:
	@$(MAKE) --directory $(StroikaRoot) --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@

project-files:
	@$(MAKE) --directory $(StroikaRoot) --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@
	@#Workaround https://stroika.atlassian.net/browse/STK-943
	@cd Workspaces/VisualStudio.Net; rm -f Microsoft.Cpp.stroika.ConfigurationBased.props; $(StroikaRoot)/ScriptsLib/MakeSymbolicLink ../../ThirdPartyComponents/Stroika/StroikaRoot/Workspaces/VisualStudio.Net/Microsoft.Cpp.stroika.ConfigurationBased.props
	@cd Workspaces/VisualStudio.Net; rm -f Microsoft.Cpp.stroika.user.props; $(StroikaRoot)/ScriptsLib/MakeSymbolicLink ../../ThirdPartyComponents/Stroika/StroikaRoot/Workspaces/VisualStudio.Net/Microsoft.Cpp.stroika.user.props

distclean:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "IPAM $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifneq ($(CONFIGURATION),)
	$(error "make distclean applies to all configurations - and deletes all configurations")
endif
	@rm -rf Builds ConfigurationFiles IntermediateFiles
	@$(MAKE) --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	-@$(MAKE) --no-print-directory --directory $(StroikaRoot) --silent $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))

clean clobber:
ifeq ($(CONFIGURATION),)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "IPAM $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifeq ($(CONFIGURATION_TAGS),)
	@if [ "$@"=="clobber" ] ; then \
		rm -rf IntermediateFiles/* Builds/*;\
	fi
	@if [ "$@" = "clean" ] ; then \
		rm -rf IntermediateFiles/*/IPAM*;\
	fi
	@# with no config specified, IPAM NYI make clean/clobber (and not needed)
	@#$(MAKE) --silent --directory IPAM $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
	@$(MAKE) --silent --directory ThirdPartyComponents $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
else
	@for i in $(APPLY_CONFIGS) ; do\
		$(MAKE) --no-print-directory $@ CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
endif
else
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "IPAM $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory LibIPAM --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory API-Server --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory Tools --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory ThirdPartyComponents --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif

## @todo cleanup - fix configs etc
run-tests:
	make --directory Tests --no-print-directory run-tests
	

ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
# This mechanism isn't strictly necessary, and slows builds slightly on windows. BUT, it can be very confusing the
# errors you get out of the box with visual studio projects without doing a make project-files, so include
# this to make the out of box experience installing Stroika a little more seemless.
# SEE https://stroika.atlassian.net/browse/STK-940
Builds/__AUTOMATIC_MAKE_PROJECT_FILES__:
	@make project-files
	@touch Builds/__AUTOMATIC_MAKE_PROJECT_FILES__
endif

update-submodules:
	git submodule update --init --recursive

STROIKA_COMMIT?=v3-Release
latest-submodules:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "IPAM $(call FUNCTION_CAPITALIZE_WORD,$@):"
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "Checkout Latest Stroika (STROIKA_COMMIT=${STROIKA_COMMIT})"
	@(cd $(StroikaRoot) && git checkout $(STROIKA_COMMIT) --quiet && git pull --quiet)

format-code:
	@$(MAKE) --directory=LibIPAM --no-print-directory format-code
	@$(MAKE) --directory=API-Server --no-print-directory format-code
	@$(MAKE) --directory=Tools --no-print-directory format-code
	@$(MAKE) --directory=Tests --no-print-directory format-code
