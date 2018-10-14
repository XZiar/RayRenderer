BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= ResourcePackager

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger
DEPLIBS		+= -lfmt -lcryptopp -lboost.stacktrace
CPPFLAGS	+= -DRESPAK_EXPORT

include $(PROJPATH)/MakeTargets.mk
