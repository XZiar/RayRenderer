#@XZBUILD
#@XZBUILD@description@A small C library for x86 CPU detection and feature extraction
#@XZBUILD@version@0.4.0

BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk


CXXFLAGS	+= -D_LIB -DHAVE_CONFIG_H -Wno-format -Wno-overlength-strings
CFLAGS      := $(filter-out -std=c11, $(CFLAGS))
NASMSRCS	 = 
NAME	= cpuid

include $(PROJPATH)/MakeTargets.mk
