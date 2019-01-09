#@XZBUILD
#@XZBUILD@description@Utility test
#@XZBUILD@dependency@fmt, stb, miniLogger, AsyncExecutor, ImageUtil, OpenGLUtil, OpenCLUtil, FreeGLUTView, StringCharset

BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= UtilTest

#CPPSRCS		 = UtilTest.cpp AsyncTest.cpp ImageUtilTest.cpp CharsetTest.cpp GLStub.cpp #$(wildcard *.cpp)
EXCEPT_CPP	+= CURLTest.cpp
LIBRARYS	+= -lpthread -lstdc++fs -lGL -lX11 -lminiLogger -lAsyncExecutor -lImageUtil -lOpenGLUtil -lOpenCLICDLoader -lOpenCLUtil -lFreeGLUTView -lStringCharset
DEPLIBS		+= -lfmt -lstb
CXXFLAGS	+= -Wno-unused-function

include $(PROJPATH)/MakeTargets.mk

