#@XZBUILD
#@XZBUILD@description@A massively spiffy yet delicately unobtrusive compression library
#@XZBUILD@version@1.2.11

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk

ifeq ($(PLATFORM), x64)
	ASMSRCS	= $(wildcard masmx64/*.S)
else
	ASMSRCS	= $(wildcard masmx86/*.S)
endif
CXXFLAGS += -D_LARGEFILE64_SOURCE=1 -DNO_UNDERLINE -DASMV -DHAVE_HIDDEN -DHAVE_UNISTD_H -DHAVE_STDARG_H
NAME	= zlib

include $(PROJPATH)/MakeTargets.mk
