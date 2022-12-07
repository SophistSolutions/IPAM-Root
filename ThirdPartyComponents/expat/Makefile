#
#
export TOP_ROOT?=$(shell realpath ../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk

include $(StroikaRoot)/ScriptsLib/SharedMakeVariables-Default.mk


ifneq ($(CONFIGURATION),)
include $(StroikaRoot)ScriptsLib/Makefile-CMake-Common.mk
endif

.PHONY:	all clean clobber check doBuilds_

#
# CHECK FOR UPDATED VERSIONS https://github.com/libexpat/libexpat/releases
#
VERSION=2_5_0
ZIPPEDFILE=../Origs-Cache/R_$(VERSION).zip


PER_CONFIGURATION_BUILDS_DIR:=$(shell realpath --canonicalize-missing ../../Builds/$(CONFIGURATION)/ThirdPartyComponents/)/

#lose trailing / on PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_ because on macos make THATDIR doesn't work with trailing slash (old WEIRD_MACOS_MAKEFILE_DIR_BUG -  MacOS XCode 10 - GNU Make 3.81)
PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_:=$(shell realpath --canonicalize-missing ../../IntermediateFiles/$(CONFIGURATION)/ThirdPartyComponents/expat)

OUTPUT_WORKDIR_PRETTYNAME="$(shell $(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/")"



FETCHURLS=
FETCHURLS+=https://github.com/libexpat/libexpat/archive/refs/tags/R_$(VERSION).zip



PRODUCED_OUTPUT_ARTIFACTS=	\
	$(PER_CONFIGURATION_BUILDS_DIR)include/expat.h \
	$(PER_CONFIGURATION_BUILDS_DIR)include/expat_config.h \
	$(PER_CONFIGURATION_BUILDS_DIR)include/expat_external.h

PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/libexpat${LIB_SUFFIX}

ifneq ($(CONFIGURATION),)
CMAKE_CURRENT_SOURCE_DIR:=$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/expat
CMAKE_CURRENT_SOURCE_DIR:=$(shell realpath --canonicalize-missing $(CMAKE_CURRENT_SOURCE_DIR) )
CMAKE_CURRENT_SOURCE_DIR:=$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(CMAKE_CURRENT_SOURCE_DIR))
endif


all:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "ThirdPartyComponents/expat $(VERSION) Build {$(CONFIGURATION)}:"
ifeq ($(CONFIGURATION),)
	@for i in `$(StroikaRoot)GetConfigurations` ; do\
		$(MAKE) --no-print-directory all CONFIGURATION=$$i;\
	done
else
	@#make zippedfile even though a dependency on CURRENT and PRODUCED_ARTIFACTS for parallel-make so completed before either starts (else one sees file partly downloaded and proceeeds)
	@$(MAKE) --silent MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $(ZIPPEDFILE)
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
ifeq ($(PRODUCED_OUTPUT_ARTIFACTS),)
	$(error This makefile requires a recognized ProjectPlatformSubdir)
endif
	@$(MAKE) --no-print-directory --silent MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $(PRODUCED_OUTPUT_ARTIFACTS)
ifneq ($(QUICK_BUILD), 1)
	@$(MAKE) --no-print-directory --silent MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) CURRENT
endif
	@$(MAKE) --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))  check
endif

#ALL produced artifacts produced by the same thing. But if you try the same rule to make all, if parallel makes invoked on this, you get
#multiple tries to run the same rule. So remap all to making the first.
$(wordlist 2, 9999, $(PRODUCED_OUTPUT_ARTIFACTS)):	$(word 1,$(PRODUCED_OUTPUT_ARTIFACTS))

$(word 1,$(PRODUCED_OUTPUT_ARTIFACTS)):
ifeq ($(CONFIGURATION),)
	$(error This makefile requires a CONFIGURATION= parameter, or environment variable set)
endif
	@$(MAKE) --silent $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
	@$(MAKE) --no-print-directory doBuilds_


clean:
ifeq ($(CONFIGURATION),)
	@rm -rf $(StroikaRoot)IntermediateFiles/*/ThirdPartyComponents/expat
else
	@rm -rf $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
endif


clobber:	clean
ifeq ($(CONFIGURATION),)
	@rm -rf $(StroikaRoot)Builds/*/ThirdPartyComponents/lib/*expat* $(StroikaRoot)Builds/*/ThirdPartyComponents/include/*expat*
	@rm -rf CURRENT
else
	@rm -fr $(PRODUCED_OUTPUT_ARTIFACTS) $(PER_CONFIGURATION_BUILDS_DIR)include/expat
endif


check:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) -n "expat ${VERSION} - Checking ... "
ifeq ($(CONFIGURATION),)
	$(error This makefile requires a CONFIGURATION= parameter, or environment variable set)
endif
ifeq ($(PRODUCED_OUTPUT_ARTIFACTS),)
	$(error This makefile requires a recognized ProjectPlatformSubdir)
endif
	@$(StroikaRoot)ScriptsLib/CheckFileExists $(PRODUCED_OUTPUT_ARTIFACTS)
	@$(ECHO) " ThirdPartyComponents/expat -  [Succeeded]";


$(ZIPPEDFILE):
	if [ ! -e ../Origs-Cache ] ; then mkdir ../Origs-Cache; fi
	@MAKE_INDENT_LEVEL=$(MAKE_INDENT_LEVEL) $(StroikaRoot)ScriptsLib/WebGet $(ZIPPEDFILE) $(FETCHURLS)


ORIGs CURRENT $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_):	$(ZIPPEDFILE)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "expat ${VERSION} - Extracting $(ZIPPEDFILE) to $@"` "... "
	@mkdir -p $@
	unzip -q $(ZIPPEDFILE) -d FRED
	mv FRED/libexpat-R_$(VERSION)/* $@
	rm -rf FRED




# JOBSERVER_INFO is the data to be assigned to MFLAGS in calling sub-make.
# MAYBE it should come from MAKEFLAGS? and maybe 
# NOTE: SUBTLE - YOU CANNOT read the MFLAGS variable from THIS MAKE, but must do it from a SUBSHELL
JOBSERVER_INFO=$(filter -j --jobserver-fds=% --jobserver-auth=%, $(shell echo $${MFLAGS}))

CMAKE_BUILD_EXTRAS_=
CMAKE_BUILD_EXTRAS_+= -v
ifneq ($(JOBSERVER_INFO),)
ifeq (Unix,$(ProjectPlatformSubdir))
# Not perfect, but maybe OK - best if cmake supported same job-server
CMAKE_BUILD_EXTRAS_ += -j10
else ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
# closest we can easily get to approximating support for job-server
export CL+=/MP
endif
endif

# On Visual Studio, they default to having this off, but we want to generate the pkgconfig file in hopes
# it helps with the build of exiv2 --LGP 2022-12-06
CMAKE_ARGS+= -DEXPAT_BUILD_PKGCONFIG=ON

CMAKE_ARGS+= -DEXPAT_SHARED_LIBS=OFF
CMAKE_ARGS+= -DEXPAT_BUILD_EXAMPLES=OFF

CMAKE_ARGS+= -DEXPAT_DEBUG_POSTFIX=
CMAKE_ARGS+= -DEXPAT_RELEASE_POSTFIX=


ifeq ($(DETECTED_HOST_OS),MSYS)
# Only needed to prevent failure with -j flag
TOOLSET_CMD_ENV_SETS_+= MAKEFLAGS= MFLAGS= CL= 
endif

doBuilds_:
	@mkdir -p $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
	@rm -f $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt && touch  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "expat ${VERSION} - Configuring (See $(OUTPUT_WORKDIR_PRETTYNAME)CONFIGURE-OUT.txt)"` "... "
	@echo CXX='$(CXX)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo CFLAGS='$(CFLAGS)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo CXXFLAGS='$(CXXFLAGS)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo 'cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) ; $(TOOLSET_CMD_ENV_SETS_)cmake -G "$(CMAKE_USE_GENERATOR)" $(CMAKE_ARGS) $(CMAKE_CURRENT_SOURCE_DIR)' >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt 2>&1
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) ; $(TOOLSET_CMD_ENV_SETS_)cmake -G "$(CMAKE_USE_GENERATOR)" $(CMAKE_ARGS) $(CMAKE_CURRENT_SOURCE_DIR) >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt 2>&1
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "expat ${VERSION} - Compiling (See $(OUTPUT_WORKDIR_PRETTYNAME)BUILD-OUT.txt)"` "... "
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(ECHO) $(TOOLSET_CMD_ENV_SETS_)"cmake --build . --config $(CMAKE_CONFIGURATION) $(CMAKE_BUILD_EXTRAS_)" > $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(TOOLSET_CMD_ENV_SETS_)cmake --build . --config $(CMAKE_CONFIGURATION) $(CMAKE_BUILD_EXTRAS_) >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "expat ${VERSION} - Copying builds (See $(OUTPUT_WORKDIR_PRETTYNAME)INSTALL.OUT)"` "... "
	@#NB: Set MAKEFLAGS= to silence warnings about -j jobserver when run with -j N (probaly bug with expat 3.2 cmake makefile but didnt dig cuz not important)
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(ECHO) $(TOOLSET_CMD_ENV_SETS_)"cmake --build . --config $(CMAKE_CONFIGURATION) --target install" > $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
	@MAKEFLAGS= && cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(TOOLSET_CMD_ENV_SETS_)cmake --build . --config $(CMAKE_CONFIGURATION) --target install >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
	@# I beleive cmake findlibrary code for FindExpat looks for libexpat.lib and builds of Exiv2 fail if that file not present
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
	@if [ ! -e $(PER_CONFIGURATION_BUILDS_DIR)lib/libexpat.lib ] ; then cp $(PER_CONFIGURATION_BUILDS_DIR)lib/libexpatd.lib $(PER_CONFIGURATION_BUILDS_DIR)lib/libexpat.lib; fi
endif
ifeq (Unix,$(ProjectPlatformSubdir))
else ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
	@#We get warnings about a missing pdb file when linking otherwise...(not sure why since symbol info sb in the .lib)
	@$(ECHO) "cp $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/src/xerces-c.dir/$(CMAKE_CONFIGURATION)/xerces*.pdb ${PER_CONFIGURATION_BUILDS_DIR}lib/ 2>/dev/null || :" >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
	@cp $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/src/xerces-c.dir/$(CMAKE_CONFIGURATION)/xerces*.pdb ${PER_CONFIGURATION_BUILDS_DIR}lib/ 2>/dev/null || :
endif


check-prerequisite-tools:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) -n "expat ${VERSION} - Checking Prerequisite tools ... "
	@{ type cmake > /dev/null 2>&1; } || { $(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) && $(StroikaRoot)ScriptsLib/GetMessageForMissingTool cmake && exit 1; }
	@$(ECHO) "done";
