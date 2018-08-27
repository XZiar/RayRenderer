BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= FontHelper

EXCEPT_CPP	+= dllmain.cpp
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLUtil
DEPLIBS		+= -lfreetype2 -lfmt
CPPFLAGS	+= -DFONTHELPER_EXPORT

include $(PROJPATH)/MakeTargets.mk
