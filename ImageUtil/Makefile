#@XZBUILD
#@XZBUILD@description@A basic image operation library
#@XZBUILD@dependency@fmt, miniLogger, zlib-ng, png, jpeg-turbo, StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= ImageUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lStringCharset
DEPLIBS		+= -lzlib-ng -lpng -ljpeg-turbo -lfmt
CPPFLAGS	+= -DIMGUTIL_EXPORT
PCH_HEADER  := ImageUtilRely.h

include $(PROJPATH)/MakeTargets.mk
