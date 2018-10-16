#@XZBUILD
#@XZBUILD@description@Wrapper of OpenGL Utility
#@XZBUILD@dependency@fmt, cpuid, miniLogger, glew, ImageUtil

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= OpenGLUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lGL
DEPLIBS		+= -lglew -lfmt -lcpuid
CPPFLAGS	+= -DOGLU_EXPORT
PCH_HEADER  := oglRely.h

include $(PROJPATH)/MakeTargets.mk
