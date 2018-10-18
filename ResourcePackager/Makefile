#@XZBUILD
#@XZBUILD@description@Resource package and management library
#@XZBUILD@dependency@fmt, miniLogger, cryptopp, boost.stacktrace

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= ResourcePackager

LIBRARYS	+= -lpthread -lstdc++fs -lminiLogger -lboost.stacktrace
DEPLIBS		+= -lfmt -lcryptopp
CPPFLAGS	+= -DRESPAK_EXPORT

include $(PROJPATH)/MakeTargets.mk
