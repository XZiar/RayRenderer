#@XZBUILD
#@XZBUILD@description@Test Program based on FreeGLUT
#@XZBUILD@dependency@fmt, FreeGLUTView, RenderCore, ResourcePackager, ImageUtil

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= GLUTTest

LIBRARYS	+= -lpthread -lstdc++fs -lFreeGLUTView -lResourcePackager -lRenderCore -lImageUtil
ifeq ($(XZMK_CLANG), 1)
LIBRARYS	+= -lminiLogger -lOpenGLUtil -lOpenCLUtil -lFontHelper -lTextureUtil -lAsyncExecutor
endif
DEPLIBS		+= -lfmt
CXXFLAGS	+= -Wno-unused-function -Wno-switch

include $(PROJPATH)/MakeTargets.mk

