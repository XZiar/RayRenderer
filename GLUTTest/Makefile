#@XZBUILD
#@XZBUILD@description@Test Program based on FreeGLUT
#@XZBUILD@dependency@fmt, FreeGLUTView, RenderCore, miniLogger

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= GLUTTest

#CPPSRCS		 = UtilTest.cpp AsyncTest.cpp ImageUtilTest.cpp CharsetTest.cpp GLStub.cpp #$(wildcard *.cpp)
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lFreeGLUTView -lTextureUtil -lResourcePackager -lRenderCore
DEPLIBS		+= -lfmt
CXXFLAGS	+= -Wno-unused-function -Wno-switch

include $(PROJPATH)/MakeTargets.mk

