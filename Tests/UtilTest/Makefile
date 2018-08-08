BUILD_TYPE	:= executable

include $(PROJPATH)/Makebase.mk

NAME	= UtilTest

CPPSRCS		 = UtilTest.cpp AsyncTest.cpp ImageUtilTest.cpp CharsetTest.cpp #$(wildcard *.cpp)
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lAsyncExecutor -lImageUtil
DEPLIBS		+= -lfmt -lstb -luchardet

include $(PROJPATH)/MakeTargets.mk

