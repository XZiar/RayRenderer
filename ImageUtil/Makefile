BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= ImageUtil

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger
DEPLIBS		+= -lzlib-ng -lpng -ljpeg-turbo -lfmt
CPPFLAGS	+= -DIMGUTIL_EXPORT

include $(PROJPATH)/MakeTargets.mk
