#@XZBUILD
#@XZBUILD@description@Texture Utility
#@XZBUILD@version@1.6.35
#@XZBUILD@dependency@fmt, miniLogger, ImageUtil, ispc_texcomp, OpenGLUtil, OpenCLUtil

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= TextureUtil

EXCEPT_CPP	+= dllmain.cpp
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLUtil
DEPLIBS		+= -lfmt -lispc_texcomp
CPPFLAGS	+= -DTEXUTIL_EXPORT

include $(PROJPATH)/MakeTargets.mk
