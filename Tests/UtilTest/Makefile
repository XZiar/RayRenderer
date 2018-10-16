#@XZBUILD
#@XZBUILD@description@Utility test
#@XZBUILD@dependency@fmt, stb, uchardet, miniLogger, AsyncExecutor, ImageUtil, OpenGLUtil, OpenCLUtil, FreeGLUTView

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= UtilTest

#CPPSRCS		 = UtilTest.cpp AsyncTest.cpp ImageUtilTest.cpp CharsetTest.cpp GLStub.cpp #$(wildcard *.cpp)
LIBRARYS	+= -lpthread -lstdc++fs -lGL -lX11 -lminiLogger -lAsyncExecutor -lImageUtil -lOpenGLUtil -lOpenCLUtil -lOpenCLICDLoader -lFreeGLUTView
DEPLIBS		+= -lfmt -lstb -luchardet
CXXFLAGS	+= -Wno-unused-function

include $(PROJPATH)/MakeTargets.mk

