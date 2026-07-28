export TOP_ROOT:=$(abspath ../../)/
StroikaRoot	:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)LibIPAM/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/IPAM/LibIPAM/Common/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk



SubDirs:=\


Objs	:=	\
	$(ObjDir)Geolocation${OBJ_SUFFIX}\


all:	
	mkdir -p $(ObjDir)
	$(MAKE) -s $(Objs)


Objs += $(SUBDIR_OBJS)


clean::
	@rm -f $(SUBDIR_OBJS)
