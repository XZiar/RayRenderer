#@XZBUILD
#@XZBUILD@description@Test Program based on FreeGLUT
#@XZBUILD@dependency@fmt, FreeGLUTView, RenderCore, ResourcePackager, ImageUtil, StringCharset

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= GLUTTest

LIBRARYS	+= -lpthread -lstdc++fs -lFreeGLUTView -lResourcePackager -lRenderCore -lImageUtil
LIBRARYS	+= -lminiLogger -lOpenGLUtil -lOpenCLICDLoader -lOpenCLUtil -lFontHelper -lTextureUtil -lAsyncExecutor -lboost.stacktrace -lStringCharset
DEPLIBS		+= -lfmt
CXXFLAGS	+= -Wno-unused-function -Wno-switch

include $(PROJPATH)/MakeTargets.mk

