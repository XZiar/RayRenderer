BUILD_TYPE	:= static

include $(PROJPATH)/Makebase.mk


CXXFLAGS	+= -D_LIB -DHAVE_CONFIG_H -Wno-format
CFLAGS      := $(filter-out -std=c11, $(CFLAGS))
NASMSRCS	 = 
NAME	= cpuid

include $(PROJPATH)/MakeTargets.mk
