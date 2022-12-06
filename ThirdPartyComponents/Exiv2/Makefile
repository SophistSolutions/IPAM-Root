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

BASENAME=main
ZIPPEDFILE=../Origs-Cache/$(BASENAME).zip
EXTRACTED_DIRNAME=$(BASENAME)


PER_CONFIGURATION_BUILDS_DIR:=$(shell realpath --canonicalize-missing ../../Builds/$(CONFIGURATION)/ThirdPartyComponents/)/

#lose trailing / on PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_ because on macos make THATDIR doesn't work with trailing slash (old WEIRD_MACOS_MAKEFILE_DIR_BUG -  MacOS XCode 10 - GNU Make 3.81)
PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_:=$(shell realpath --canonicalize-missing ../../IntermediateFiles/$(CONFIGURATION)/ThirdPartyComponents/Exiv2)

OUTPUT_WORKDIR_PRETTYNAME="$(shell $(StroikaRoot)ScriptsLib/SubstituteBackVariables "$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/")"
#OUTPUT_WORKDIR_PRETTYNAME="{StroikaRoot}Inter.../Thir.../Xerces/"


#
# CHECK FOR UPDATED VERSIONS http://xerces.apache.org/xerces-c/
#

FETCHURLS=
FETCHURLS+=https://github.com/Exiv2/exiv2/archive/refs/heads/$(BASENAME).zip




PRODUCED_OUTPUT_ARTIFACTS=	\
	$(PER_CONFIGURATION_BUILDS_DIR)include/exiv2/value.hpp

ifeq (Unix,$(ProjectPlatformSubdir))
PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/libexiv2.a
endif
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
ifeq (Debug,$(CMAKE_CONFIGURATION))
PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/exiv2.lib
endif
ifeq (Release,$(CMAKE_CONFIGURATION))
PRODUCED_OUTPUT_ARTIFACTS+=	\
	$(PER_CONFIGURATION_BUILDS_DIR)lib/exiv2.lib
endif
endif

ifneq ($(CONFIGURATION),)
CMAKE_CURRENT_SOURCE_DIR:=$(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)
CMAKE_CURRENT_SOURCE_DIR:=$(shell realpath --canonicalize-missing $(CMAKE_CURRENT_SOURCE_DIR) )
CMAKE_CURRENT_SOURCE_DIR:=$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(CMAKE_CURRENT_SOURCE_DIR))
endif

ifeq (true,$(HasMakefileBugWorkaround_lto_skipping_undefined_incompatible))
### not sure why???
#Building Stroika Tools {clang++-6-release-libstdc++}:
#         Compiling {StroikaRoot}Tools/Sources/Stroika/Frameworks/WebServer/HTMLViewCompiler/HTMLViewCompiler.cpp ...
#      Linking  $StroikaRoot/Builds/clang++-6-release-libstdc++/HTMLViewCompiler...
#/usr/bin/ld: skipping incompatible /Sandbox/Stroika-Dev/Builds/clang++-6-release-libstdc++/ThirdPartyComponents/lib//libxerces-c.so when searching for -lxerces-c
CFLAGS:=$(filter-out -flto, $(CFLAGS))
CXXFLAGS:=$(filter-out -flto, $(CXXFLAGS))
else ifeq (Unix,$(ProjectPlatformSubdir))
## similar bug with g++ - but just warnings here (all regtests pass without this change)
##/home/lewis/Sandbox/Stroika-Build-Dir-Ubuntu1804_x86_64/Builds/g++-optimized/ThirdPartyComponents/include/xercesc/util/BinInputStream.hpp:29:22: warning: type 'struct BinInputStream' violates the C++ One Definition Rule [-Wodr]
CFLAGS:=$(filter-out -flto, $(CFLAGS))
CXXFLAGS:=$(filter-out -flto, $(CXXFLAGS))
endif


ifeq (Unix,$(ProjectPlatformSubdir))
#@todo fixup - bug workaround - configure autodetects -posix and se we have to add it in here. Must find
# some way we can ADD these params to CFLAGS without REPLACING
# -- without this hack - we get link error building Xerces
CFLAGS+= -pthread
CXXFLAGS+= -pthread
endif


##
## It appears just /I"C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/ucrt"
## causes build problems. Unsure why (didnt did much).
##
## But there is no need for any of the /I parameters, as none were supported before Stroika 2.1a2, so strip them all for now
##		-- LGP 2019-09-26
##
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
CFLAGS:=$(call STRIP_INCLUDE_COMPILER_FLAGS,$(CFLAGS))
CXXFLAGS:=$(call STRIP_INCLUDE_COMPILER_FLAGS,$(CXXFLAGS))
endif


# Avoid having warnings leak from here to linking code when Xerces is used --LGP 2022-01-18
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
	@rm -rf $(EXTRACTED_DIRNAME) 
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
	@mkdir -p $@
	unzip -q $(ZIPPEDFILE) -d FRED
	mv FRED/exiv2-main/* $@
	rm -rf FRED
	if [ "$@" != "ORIGs" ] ; then \
		$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Exiv2 ${VERSION} - Patching ... ";\
		patch -p0 $@/cmake/compilerFlags.cmake Patches/Patch-KillCompilerFlagChangeBreakingStaticLibsWindoze.Patch >> $@/PATCHING.OUT;\
	fi





###NOTE - because we set network:BOOL:OFF - Xerces doesn't depend on CURL (but we could make it so if we wanted, but then update dependencies
### in ThirdPartyLibs/Makefile
# CMAKE_ARGS+= -Dnetwork:BOOL=OFF
# ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
# CMAKE_ARGS+= -Dtranscoder=windows
# else ifeq ($(DETECTED_HOST_OS),Darwin)
# #AVOID Undefined symbols for architecture x86_64:  "_CFRelease", etc.... Avoid library dependency
# CMAKE_ARGS+= -Dtranscoder=iconv
# else
# CMAKE_ARGS+= -Dtranscoder=gnuiconv
# endif

## DIDNT HELP
#CMAKE_ARGS+= -DCMAKE_SYSTEM_PREFIX_PATH=$(CMAKE_PER_TARGET_BUILD_DIR)


#   Checking Build System
#   Building Custom Rule C:/Sandbox/IPAM/DevRoot/IntermediateFiles/Debug/ThirdPartyComponents/Exiv2/xmpsdk/CMakeLists.txt
# cl : command line  error D8016: '/Ox' and '/RTCs' command-line options are incompatible [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\xmpsdk\exiv2-xmp.vcxproj]
#   Building Custom Rule C:/Sandbox/IPAM/DevRoot/IntermediateFiles/Debug/ThirdPartyComponents/Exiv2/src/CMakeLists.txt
# cl : command line  error D8016: '/Ox' and '/RTCs' command-line options are incompatible [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\src\exiv2lib_int.vcxproj]
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
CFLAGS:=$(filter-out -RTCs%, $(CFLAGS))
CXXFLAGS:=$(filter-out -RTCs%, $(CXXFLAGS))
endif


#           ] (compiling source file C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\src\cr2image.cpp)
# C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\src\futils.cpp(341,7): error C2679: binary '=': no operator found which takes a right-hand operand of type 'TCHAR [260]' (or there is no acceptable conversion) [C:\Sandbox\IPAM\DevRoot\IntermediateFiles\Debug\ThirdPartyComponents\Exiv2\src\exiv2lib.vcxproj]
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))
CFLAGS:=$(filter-out -DUNICODE, $(CFLAGS))
CXXFLAGS:=$(filter-out -DUNICODE, $(CXXFLAGS))
CFLAGS:=$(filter-out -D_UNICODE, $(CFLAGS))
CXXFLAGS:=$(filter-out -D_UNICODE, $(CXXFLAGS))
endif


CMAKE_ARGS+= -DEXIV2_ENABLE_BROTLI=no

CMAKE_ARGS+= -DBUILD_SHARED_LIBS=no


### Should not be needed but finding stuff in wrong place - mingw - instead...
###  SB mingw only hack at least
ifeq (VisualStudio.Net,$(findstring VisualStudio.Net,$(ProjectPlatformSubdir)))

X_=$(call FUNCTION_CONVERT_FILES_TO_COMPILER_NATIVE,$(PER_CONFIGURATION_BUILDS_DIR))
CMAKE_ARGS+= -DPC_EXPAT_INCLUDE_DIRS=$(X_)/include/
CMAKE_ARGS+= -DPC_EXPAT_LIBRARY_DIRS=$(X_)/lib/
CMAKE_ARGS+= -DZLIB_INCLUDE_DIR=$(X_)/include/
CMAKE_ARGS+= -DZLIB_LIBRARY_DIR=$(X_)/lib/
CMAKE_ARGS+= -DZLIB_LIBRARY_RELEASE=$(X_)/lib/zlib.lib
endif



##
### Avoid compile error in time_t XMLDateTime::getEpoch(bool duration) const
### not sure why this is needed?
###  -- LGP 2019-09-26
##
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

ifneq ($(JOBSERVER_INFO),)
ifeq (Unix,$(ProjectPlatformSubdir))
# Trick to make gnu make JOBSERVER work with cmake - pass args to trampoline with environment variables; doesn't work on darwin(debug? lowpri)
export MAKE
CMAKE_ARGS+= -DCMAKE_MAKE_PROGRAM="$(shell realpath MAKE-Trampoline)"
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
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(ECHO) $(TOOLSET_CMD_ENV_SETS_)"cmake --build . --config $(CMAKE_CONFIGURATION)" > $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
	@cd $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_) && $(TOOLSET_CMD_ENV_SETS_)cmake --build . --config $(CMAKE_CONFIGURATION) >> $(PER_CONFIGURATION_THIS_INTERMEDIATEFILES_DIR_NOSLASH_)/BUILD-OUT.txt 2>&1
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
