BUILD_TYPE	:= static

include $(PROJPATH)Makebase.mk

SUBDIRS		+= $(OBJPATH)masmx64 $(OBJPATH)masmx86
ifeq ($(PLATFORM), x64)
	ASMSRCS	= $(wildcard masmx64/*.S)
else
	ASMSRCS	= $(wildcard masmx86/*.S)
endif
CXXFLAGS += -D_LARGEFILE64_SOURCE=1 -DNO_UNDERLINE -DASMV
NAME	= zlibstatic

include $(PROJPATH)MakeTargets.mk
