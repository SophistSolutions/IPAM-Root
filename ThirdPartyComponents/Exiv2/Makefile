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
# CHECK FOR UPDATED VERSIONS https://github.com/Exiv2/exiv2/releases
#
#		NOTE - main not a real release number - so this isn't safe. Floating version
#		But they dont appear to have done a release since fixing the auto_ptr stuff
#VERSION=main
VERSION=3dd881fb122835605ba011413ec86c121101d131
#VERSION=0.27.5
ZIPPEDFILE=../Origs-Cache/Exiv2_$(VERSION).zip




PER_CONFIGURATION_BUILDS_DIR:=$(shell realpath --canonicalize-missing ../../Builds/$(CONFIGURATION)/ThirdPartyComponents/)/

#lose trailing / on PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_ because on macos make THATDIR doesn't work with trailing slash (old WEIRD_MACOS_MAKEFILE_DIR_BUG -  MacOS XCode 10 - GNU Make 3.81)
PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_:=$(shell realpath --canonicalize-missing ../../IntermediateFiles/$(CONFIGURATION)/ThirdPartyComponents/Exiv2)

OUTPUT_WORKDIR_PRETTYNAME="$(shell $(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/")"


#
#
#
FETCHURLS=
FETCHURLS+=https://github.com/Exiv2/exiv2/archive/refs/tags/v$(VERSION).zip
FETCHURLS+=https://github.com/Exiv2/exiv2/archive/refs/heads/$(VERSION).zip
FETCHURLS+=https://github.com/Exiv2/exiv2/archive/$(VERSION).zip




PRODUCED_OUTPUT_ARTIFACTS=	\
	$(PER_CONFIGURATION_BUILDS_DIR)include/exiv2/value.hpp

ifeq (Unix,$(ProjectPlatformSubdir))
PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/libexiv2.a
endif
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/exiv2${LIB_SUFFIX}
endif

ifneq ($(CONFIGURATION),)
CMAKE_CURRENT_SOURCE_DIR:=$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
CMAKE_CURRENT_SOURCE_DIR:=$(shell realpath --canonicalize-missing $(CMAKE_CURRENT_SOURCE_DIR) )
CMAKE_CURRENT_SOURCE_DIR:=$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(CMAKE_CURRENT_SOURCE_DIR))
endif

# Avoid having warnings leak from here to linking code when Exiv is used (linktime codegen) --LGP 2022-01-18
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
ifneq ($(findstring -GL,$(CXXFLAGS)),)
CXXFLAGS+= -wd4702 
endif
endif


all:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "ThirdPartyComponents/Exiv2 $(VERSION) Build {$(CONFIGURATION)}:"
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
	@rm -rf $(StroikaRoot)IntermediateFiles/*/ThirdPartyComponents/Exiv2
else
	@rm -rf $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
endif


clobber:	clean
ifeq ($(CONFIGURATION),)
	@rm -rf $(StroikaRoot)Builds/*/ThirdPartyComponents/lib/*Exiv2* $(StroikaRoot)Builds/*/ThirdPartyComponents/include/*Exiv2*
	@rm -rf CURRENT
else
	@rm -fr $(PRODUCED_OUTPUT_ARTIFACTS) $(PER_CONFIGURATION_BUILDS_DIR)include/Exiv2
endif


check:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) -n "Exiv2 ${VERSION} - Checking ... "
ifeq ($(CONFIGURATION),)
	$(error This makefile requires a CONFIGURATION= parameter, or environment variable set)
endif
ifeq ($(PRODUCED_OUTPUT_ARTIFACTS),)
	$(error This makefile requires a recognized ProjectPlatformSubdir)
endif
	@$(StroikaRoot)ScriptsLib/CheckFileExists $(PRODUCED_OUTPUT_ARTIFACTS)
	@$(ECHO) " Stroika/ThirdPartyComponents/Exiv2 -  [Succeeded]";


$(ZIPPEDFILE):
	if [ !-e ../Origs-Cache ] ; then mkdir ../Origs-Cache; fi
	@MAKE_INDENT_LEVEL=$(MAKE_INDENT_LEVEL) $(StroikaRoot)ScriptsLib/WebGet $(ZIPPEDFILE) $(FETCHURLS)


ORIGs CURRENT $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_):	$(ZIPPEDFILE)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "Exiv2 ${VERSION} - Extracting $(ZIPPEDFILE) to $@"` "... "
	@rm -rf FRED $@
	@mkdir $@
	@unzip -q $(ZIPPEDFILE) -d FRED
	@mv FRED/exiv2-$(VERSION)/* $@
	@rm -rf FRED
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
ifeq ($(VERSION),main)
	if [ "$@" != "ORIGs" ] ; then \
		$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Exiv2 ${VERSION} - Patching ($@/PATCHING.OUT)... ";\
		patch -p0 $@/src/futils.cpp Patches/BeCompatibleWithUNICODEAPIDefines.Patch >> $@/PATCHING.OUT;\
		patch -p0 $@/cmake/compilerFlags.cmake Patches/Patch-KillCompilerFlagChangeBreakingStaticLibsWindoze.Patch >> $@/PATCHING.OUT;\
	fi
endif
endif



# Just because we don't build that library
CMAKE_ARGS+= -DEXIV2_ENABLE_BROTLI=OFF

# https://github.com/Exiv2/exiv2#BMFF
CMAKE_ARGS+= -DEXIV2_ENABLE_BMFF=ON

# I personally prefer static linking
CMAKE_ARGS+= -DBUILD_SHARED_LIBS=OFF

## Should not be needed but finding stuff in wrong place - mingw - instead...
##  SB mingw only hack at least
## https://github.com/SophistSolutions/IPAM-Root/issues/9
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
X_=$(call FUNCTION_CONVERT_FILES_TO_COMPILER_NATIVE,$(PER_CONFIGURATION_BUILDS_DIR))
CMAKE_ARGS+= -DPC_EXPAT_INCLUDE_DIRS=$(X_)/include/
CMAKE_ARGS+= -DPC_EXPAT_LIBRARY_DIRS=$(X_)/lib/
CMAKE_ARGS+= -DZLIB_INCLUDE_DIR=$(X_)/include/
CMAKE_ARGS+= -DZLIB_LIBRARY_DIR=$(X_)/lib/
CMAKE_ARGS+= -DZLIB_LIBRARY_RELEASE=$(X_)/lib/zlib.lib
endif



##
### not sure why this is needed?
# BUT AVOID THIS:
#   XMPUtils-FileInfo.cpp
#   XMPUtils.cpp
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPCore_Impl.hpp(29,11): fatal  error C1083: Cannot open include file: 'pthread.h': No such file or directory (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\ExpatAdapter.cpp) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPCore_Impl.hpp(29,11): fatal  error C1083: Cannot open include file: 'pthread.h': No such file or directory (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPMeta-Parse.cpp) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPCore_Impl.hpp(29,11): fatal  error C1083: Cannot open include file: 'pthread.h': No such file or directory (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPIterator.cpp) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPCore_Impl.hpp(29,11): fatal  error C1083: Cannot open include file: 'pthread.h': No such file or directory (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPMeta.cpp) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\XMPCore_Impl.hpp(29,11): fatal  error C1083: Cannot open include file: 'pthread.h': No such file or directory (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\src\WXMPUtils.cpp) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
###  -- LGP 2022-12=07
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
CFLAGS += /DWIN32
CXXFLAGS += /DWIN32
endif


#
#PARALLEL BUILD TRICKERY
#	We CANNOT find out the argument to -j (-jN). But even if we could, we would
#	really need to shared the same job-server in any other tasks which is captured in $(JOBSERVER_INFO)
#	Really - for now - just use if {JOBSERVER_INFO} is empty or not as a flag for whether or not to do any
#	parallel build magic (windoze and unix)
#		-- LGP 2019-01-10
#

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

ifeq ($(DETECTED_HOST_OS),MSYS)
# Only needed to prevent failure with -j flag
TOOLSET_CMD_ENV_SETS_+= MAKEFLAGS= MFLAGS= CL= 
endif

doBuilds_:
	@mkdir -p $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
	@rm -f $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt && touch  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "Exiv2 ${VERSION} - Configuring (See $(OUTPUT_WORKDIR_PRETTYNAME)CONFIGURE-OUT.txt)"` "... "
	@echo CXX='$(CXX)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo CFLAGS='$(CFLAGS)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo CXXFLAGS='$(CXXFLAGS)' >>  $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt
	@echo 'cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) ; $(TOOLSET_CMD_ENV_SETS_)cmake -G "$(CMAKE_USE_GENERATOR)" $(CMAKE_ARGS) $(CMAKE_CURRENT_SOURCE_DIR)' >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt 2>&1
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) ; $(TOOLSET_CMD_ENV_SETS_)cmake -G "$(CMAKE_USE_GENERATOR)" $(CMAKE_ARGS) $(CMAKE_CURRENT_SOURCE_DIR) >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/CONFIGURE-OUT.txt 2>&1
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "Exiv2 ${VERSION} - Compiling (See $(OUTPUT_WORKDIR_PRETTYNAME)BUILD-OUT.txt)"` "... "
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(ECHO) $(TOOLSET_CMD_ENV_SETS_)"cmake --build . --config $(CMAKE_CONFIGURATION) $(CMAKE_BUILD_EXTRAS_)" > $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(TOOLSET_CMD_ENV_SETS_)cmake --build . --config $(CMAKE_CONFIGURATION) $(CMAKE_BUILD_EXTRAS_)>> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) `$(StroikaRoot)ScriptsLib/SubstituteBackVariables "Exiv2 ${VERSION} - Copying builds (See $(OUTPUT_WORKDIR_PRETTYNAME)INSTALL.OUT)"` "... "
	@#NB: Set MAKEFLAGS= to silence warnings about -j jobserver when run with -j N (probaly bug with Exiv2 3.2 cmake makefile but didnt dig cuz not important)
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(ECHO) $(TOOLSET_CMD_ENV_SETS_)"cmake --build . --config $(CMAKE_CONFIGURATION) --target install" > $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
	@MAKEFLAGS= && cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(TOOLSET_CMD_ENV_SETS_)cmake --build . --config $(CMAKE_CONFIGURATION) --target install >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
ifeq (Unix,$(ProjectPlatformSubdir))
else ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
	@#We get warnings about a missing pdb file when linking otherwise...(not sure why since symbol info sb in the .lib)
	@$(ECHO) "cp $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/src/exiv2s-c.dir/$(CMAKE_CONFIGURATION)/exiv2s*.pdb ${PER_CONFIGURATION_BUILDS_DIR}lib/ 2>/dev/null || :" >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/INSTALL.OUT
	@cp $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/src/exiv2s-c.dir/$(CMAKE_CONFIGURATION)/exiv2s*.pdb ${PER_CONFIGURATION_BUILDS_DIR}lib/ 2>/dev/null || :
endif



check-prerequisite-tools:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) -n "Exiv2 ${VERSION} - Checking Prerequisite tools ... "
	@{ type cmake > /dev/null 2>&1; } || { $(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) && $(StroikaRoot)ScriptsLib/GetMessageForMissingTool cmake && exit 1; }
	@$(ECHO) "done";
