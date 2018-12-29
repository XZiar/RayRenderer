#@XZBUILD
#@XZBUILD@description@Renderer core
#@XZBUILD@dependency@fmt, miniLogger, ImageUtil, OpenGLUtil, OpenCLUtil, boost.stacktrace, ResourcePackager, AsyncExecutor, FontHelper, TextureUtil, StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= RenderCore

EXCEPT_CPP	+= dllmain.cpp ShadowMapping.cpp
CPPSRCS	 	+= $(wildcard Model/*.cpp)
INCPATH		+= -I.
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLUtil -lAsyncExecutor -lFontHelper -lTextureUtil -lboost.stacktrace -lStringCharset
LIBRARYS	+= -lResourcePackager
DEPLIBS		+= -lfmt
CPPFLAGS	+= -DRAYCORE_EXPORT
PCH_HEADER  := RenderCoreRely.h

include $(PROJPATH)/MakeTargets.mk
