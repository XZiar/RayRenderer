#@XZBUILD
#@XZBUILD@description@Wrapper of OpenCL Utility
#@XZBUILD@dependency@fmt, OpenGLUtil, miniLogger, glew, ImageUtil, OpenCLICDLoader, StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= OpenCLUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLICDLoader -lStringCharset
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DOCLU_EXPORT
PCH_HEADER  := oclRely.h

include $(PROJPATH)/MakeTargets.mk
