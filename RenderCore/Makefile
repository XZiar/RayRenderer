#@XZBUILD
#@XZBUILD@description@Renderer core
#@XZBUILD@dependency@fmt, miniLogger, ImageUtil, OpenGLUtil, OpenCLUtil, boost.stacktrace, ResourcePackager, uchardet, AsyncExecutor, FontHelper, TextureUtil

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= RenderCore

EXCEPT_CPP	+= dllmain.cpp ShadowMapping.cpp
CPPSRCS	 	+= $(wildcard Model/*.cpp)
INCPATH		+= -I.
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLUtil -lAsyncExecutor -lResourcePackager -lFontHelper -lTextureUtil
DEPLIBS		+= -lfmt -lboost.stacktrace -luchardet
CPPFLAGS	+= -DRAYCORE_EXPORT

include $(PROJPATH)/MakeTargets.mk
