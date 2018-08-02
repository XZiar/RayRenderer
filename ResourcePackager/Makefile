BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= ResourcePackager

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger
DEPLIBS		+= -lfmt -lcryptopp
CPPFLAGS	+= -DRESPAK_EXPORT

include $(PROJPATH)/MakeTargets.mk
