BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= OpenCLUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLICDLoader
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DOCLU_EXPORT
PCH_HEADER  := oclRely.h

include $(PROJPATH)/MakeTargets.mk
