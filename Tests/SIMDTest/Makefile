#@XZBUILD
#@XZBUILD@description@SIMD test
#@XZBUILD@dependency@fmt, cpuid

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= SIMDTest

DEPLIBS		+= -lfmt -lcpuid
CXXFLAGS	+= -Wno-unused-function

include $(PROJPATH)/MakeTargets.mk

