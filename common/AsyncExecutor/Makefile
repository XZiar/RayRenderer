#@XZBUILD
#@XZBUILD@description@An async task execution library
#@XZBUILD@dependency@fmt, miniLogger, boost.context, StringCharset

BUILD_TYPE	:= dynamic

include $(PROJPATH)/Makebase.mk

NAME	= AsyncExecutor

LIBRARYS	+= -lpthread -lminiLogger -lStringCharset
DEPLIBS		+= -lboost.context
CPPFLAGS	+= -DASYEXE_EXPORT

include $(PROJPATH)/MakeTargets.mk
