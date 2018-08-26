BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= OpenCLUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLICDLoader
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DOCLU_EXPORT

include $(PROJPATH)/MakeTargets.mk
