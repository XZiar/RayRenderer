#@XZBUILD
#@XZBUILD@description@Font Utility
#@XZBUILD@dependency@fmt, miniLogger, ImageUtil, freetype2, OpenGLUtil, OpenCLUtil, StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= FontHelper

EXCEPT_CPP	+= dllmain.cpp
LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lImageUtil -lOpenGLUtil -lOpenCLUtil -lStringCharset
DEPLIBS		+= -lfreetype2 -lfmt
CPPFLAGS	+= -DFONTHELPER_EXPORT

include $(PROJPATH)/MakeTargets.mk
