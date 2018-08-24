BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= SIMDTest

DEPLIBS		+= -lfmt -lcpuid
CXXFLAGS	+= -Wno-unused-function

include $(PROJPATH)/MakeTargets.mk

